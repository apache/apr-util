/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Cryptographic Pseudo Random Number Generator (CPRNG), based on
 * "Fast-key-erasure random-number generators" from D.J. Bernstein ([1]),
 * and public domain implementation in libpqcrypto's randombytes() ([2]).
 *
 * The CPRNG key is changed as soon as it's used to initialize the stream
 * cipher, so it never resides in memory at the same time as the keystream
 * it produced (a.k.a. the random bytes, which for efficiency are pooled).
 *
 * Likewise, the keystream is always cleared from the internal state before
 * being returned to the user, thus there is no way to recover the produced
 * random bytes afterward (e.g. from a memory/core dump after a crash).
 *
 * IOW, this CPRNG ensures forward secrecy, one may want to run it in a process
 * and/or environment protected from live memory eavesdropping, thus keep the
 * pooled/future random bytes secret by design, and use it as a replacement
 * for some blocking/inefficient system RNG. The random bytes could then be
 * serviced through a named pipe/socket, RPC, or any specific API. This is
 * outside the scope of this/below code, though.
 *
 * [1] https://blog.cr.yp.to/20170723-random.html
 * [2] https://libpqcrypto.org/
 */

#include "apu.h"

#include "apr_crypto.h"

#if APU_HAVE_CRYPTO
#if APU_HAVE_CRYPTO_PRNG

#include "apr_ring.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_thread_mutex.h"
#include "apr_thread_proc.h"

#include <stdlib.h> /* for malloc() */

#define CPRNG_KEY_SIZE 32

/* Be consistent with the .h (the seed is xor-ed with key on reseed). */
#if CPRNG_KEY_SIZE != APR_CRYPTO_PRNG_SEED_SIZE
#error apr_crypto_prng handles stream ciphers with 256bit keys only
#endif

#define CPRNG_BUF_SIZE_MIN (CPRNG_KEY_SIZE * (8 - 1))
#define CPRNG_BUF_SIZE_DEF (CPRNG_KEY_SIZE * (24 - 1))

#if APU_HAVE_OPENSSL

#include <openssl/evp.h>
#include <openssl/sha.h>

/* We only handle Chacha20 and AES256-CTR stream ciphers, for now.
 * AES256-CTR should be in any openssl version of this century but is used
 * only if Chacha20 is missing (openssl < 1.1). This is because Chacha20 is
 * fast (enough) in software and timing attacks safe, though AES256-CTR can
 * be faster and constant-time but only when the CPU (aesni) or some crypto
 * hardware are in the place.
 */
#include <openssl/obj_mac.h> /* for NID_* */
#if !defined(NID_chacha20) && !defined(NID_aes_256_ctr)
/* XXX: APU_HAVE_CRYPTO_PRNG && APU_HAVE_OPENSSL shoudn't be defined! */
#error apr_crypto_prng needs OpenSSL implementation for Chacha20 or AES256-CTR
#endif

typedef EVP_CIPHER_CTX cprng_stream_ctx_t;

static apr_status_t cprng_lib_init(apr_pool_t *pool)
{
    return apr_crypto_lib_init("openssl", NULL, NULL, pool);
}

static apr_status_t cprng_stream_ctx_make(cprng_stream_ctx_t **pctx)
{
    EVP_CIPHER_CTX *ctx;
    const EVP_CIPHER *cipher;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return APR_ENOMEM;
    }

#if defined(NID_chacha20)
    cipher = EVP_chacha20();
#else
    cipher = EVP_aes_256_ctr();
#endif
    if (EVP_EncryptInit_ex(ctx, cipher, NULL, NULL, NULL) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return APR_ENOMEM;
    }

    *pctx = ctx;
    return APR_SUCCESS;
}

static APR_INLINE
void cprng_stream_ctx_free(cprng_stream_ctx_t *ctx)
{
    EVP_CIPHER_CTX_free(ctx);
}

static APR_INLINE
apr_status_t cprng_stream_ctx_bytes(cprng_stream_ctx_t **pctx,
                                    unsigned char *key, unsigned char *to,
                                    apr_size_t n, const unsigned char *z)
{
    cprng_stream_ctx_t *ctx = *pctx;
    int len;

    /* Can be called for rekeying (key != NULL), streaming more bytes
     * with the current key (n != 0), or both successively in a one go.
     */
    if (key) {
        /* We never encrypt twice with the same key, so we can use an
         * all zeros IV. Also EVP_EncryptInit() can be called multiple
         * times and it will recycle its previous/replaced resources by
         * itself, so since padding is disabled/irrelevant too we don't
         * need EVP_EncryptFinish() after each call or before rekeying.
         */
#if defined(NID_chacha20)
        /* With CHACHA20, iv=NULL is the same as zeros but it's faster
         * to (re-)init; use that for efficiency.
         */
        EVP_EncryptInit_ex(ctx, NULL, NULL, key, NULL);
#else
        /* With AES256-CTR, iv=NULL seems to peek up and random one (for
         * the initial CTR), while we can live with zeros (fixed CTR);
         * efficiency still.
         */
        EVP_EncryptInit_ex(ctx, NULL, NULL, key, z);
#endif
        EVP_CIPHER_CTX_set_padding(ctx, 0);

        memset(key, 0, CPRNG_KEY_SIZE);
        EVP_EncryptUpdate(ctx, key, &len, key, CPRNG_KEY_SIZE);
    }
    if (n) {
        EVP_EncryptUpdate(ctx, to, &len, z, n);
    }

    return APR_SUCCESS;
}

#else /* APU_HAVE_OPENSSL */

/* XXX: APU_HAVE_CRYPTO_PRNG shoudn't be defined! */
#error apr_crypto_prng implemented with OpenSSL only for now

#endif /* APU_HAVE_OPENSSL */

struct apr_crypto_prng_t {
    APR_RING_ENTRY(apr_crypto_prng_t) link;
    apr_pool_t *pool;
    cprng_stream_ctx_t *ctx;
#if APR_HAS_THREADS
    apr_thread_mutex_t *mutex;
#endif
    unsigned char *key, *buf;
    apr_size_t len, pos;
    int flags;
};

static apr_crypto_prng_t *cprng_global = NULL;
static APR_RING_HEAD(apr_cprng_ring, apr_crypto_prng_t) *cprng_ring;

#if APR_HAS_THREADS
static apr_thread_mutex_t *cprng_ring_mutex;

static apr_threadkey_t *cprng_thread_key = NULL;

#define cprng_lock(g) \
    if ((g)->mutex) \
        apr_thread_mutex_lock((g)->mutex)

#define cprng_unlock(g) \
    if ((g)->mutex) \
        apr_thread_mutex_unlock((g)->mutex)

#define cprng_ring_lock() \
    if (cprng_ring_mutex) \
        apr_thread_mutex_lock(cprng_ring_mutex)

#define cprng_ring_unlock() \
    if (cprng_ring_mutex) \
        apr_thread_mutex_unlock(cprng_ring_mutex)

static void cprng_thread_destroy(void *cprng)
{
    if (!cprng_global) {
        apr_threadkey_private_delete(cprng_thread_key);
        cprng_thread_key = NULL;
    }
    apr_crypto_prng_destroy(cprng);
}

#else  /* !APR_HAS_THREADS */
#define cprng_lock(g)
#define cprng_unlock(g)
#define cprng_ring_lock()
#define cprng_ring_unlock()
#endif /* !APR_HAS_THREADS */

APR_DECLARE(apr_status_t) apr_crypto_prng_init(apr_pool_t *pool,
                                               apr_size_t bufsize,
                                               const unsigned char seed[],
                                               int flags)
{
    apr_status_t rv;

    if (cprng_global) {
        return APR_EREINIT;
    }

    rv = cprng_lib_init(pool);
    if (rv != APR_SUCCESS && rv != APR_EREINIT) {
        return rv;
    }

    if (flags & APR_CRYPTO_PRNG_PER_THREAD) {
#if !APR_HAS_THREADS
        return APR_ENOTIMPL;
#else
        rv = apr_threadkey_private_create(&cprng_thread_key,
                                          cprng_thread_destroy, pool);
        if (rv != APR_SUCCESS) {
            return rv;
        }
#endif
    }

    cprng_ring = apr_palloc(pool, sizeof(*cprng_ring));
    if (!cprng_ring) {
        return APR_ENOMEM;
    }
    APR_RING_INIT(cprng_ring, apr_crypto_prng_t, link);

#if APR_HAS_THREADS
    rv = apr_thread_mutex_create(&cprng_ring_mutex, APR_THREAD_MUTEX_DEFAULT,
                                 pool);
    if (rv != APR_SUCCESS) {
        apr_threadkey_private_delete(cprng_thread_key);
        cprng_thread_key = NULL;
        return rv;
    }

    /* Global CPRNG is locked (and obviously not per-thread) */
    flags = (flags | APR_CRYPTO_PRNG_LOCKED) & ~APR_CRYPTO_PRNG_PER_THREAD;
#endif

    return apr_crypto_prng_create(&cprng_global, bufsize, flags, seed, pool);
}

APR_DECLARE(apr_status_t) apr_crypto_prng_term(void)
{
    if (!cprng_global) {
        return APR_EINIT;
    }

    apr_crypto_prng_destroy(cprng_global);
    cprng_global = NULL;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_random_bytes(void *buf, apr_size_t len)
{
    if (!cprng_global) {
        return APR_EINIT;
    }

    return apr_crypto_prng_bytes(cprng_global, buf, len);
}

#if APR_HAS_THREADS
APR_DECLARE(apr_status_t) apr_crypto_random_thread_bytes(void *buf,
                                                         apr_size_t len)
{
    apr_status_t rv;
    apr_crypto_prng_t *cprng;
    void *private = NULL;

    if (!cprng_thread_key) {
        return APR_EINIT;
    }

    rv = apr_threadkey_private_get(&private, cprng_thread_key);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    cprng = private;
    if (!cprng) {
        rv = apr_crypto_prng_create(&cprng, 0, APR_CRYPTO_PRNG_PER_THREAD,
                                    NULL, NULL);
        if (rv != APR_SUCCESS) {
            return rv;
        }

        rv = apr_threadkey_private_set(cprng, cprng_thread_key);
        if (rv != APR_SUCCESS) {
            apr_crypto_prng_destroy(cprng);
            return rv;
        }
    }

    return apr_crypto_prng_bytes(cprng, buf, len);
}
#endif

static apr_status_t cprng_cleanup(void *arg)
{
    apr_crypto_prng_t *cprng = arg;

    if (cprng == cprng_global) {
        cprng_global = NULL;
#if APR_HAS_THREADS
        cprng_ring_mutex = NULL;
#endif
        cprng_ring = NULL;
    }
    else if (cprng_global && !(cprng->flags & APR_CRYPTO_PRNG_PER_THREAD)) {
        cprng_ring_lock();
        APR_RING_REMOVE(cprng, link);
        cprng_ring_unlock();
    }

    if (cprng->ctx) {
        cprng_stream_ctx_free(cprng->ctx);
    }

    if (cprng->key) {
        apr_crypto_memzero(cprng->key, CPRNG_KEY_SIZE + cprng->len);
    }

    if (!cprng->pool) {
        free(cprng->key);
        free(cprng);
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_create(apr_crypto_prng_t **pcprng,
                                                 apr_size_t bufsize, int flags,
                                                 const unsigned char seed[],
                                                 apr_pool_t *pool)
{
    apr_status_t rv;
    apr_crypto_prng_t *cprng;

    *pcprng = NULL;

    if (!cprng_global && pcprng != &cprng_global) {
        return APR_EINIT;
    }

    if (bufsize > APR_INT32_MAX - CPRNG_KEY_SIZE
            || (flags & APR_CRYPTO_PRNG_LOCKED && !pool)
            || (flags & ~APR_CRYPTO_PRNG_MASK)) {
        return APR_EINVAL;
    }

#if !APR_HAS_THREADS
    if (flags & (APR_CRYPTO_PRNG_LOCKED | APR_CRYPTO_PRNG_PER_THREAD)) {
        return APR_ENOTIMPL;
    }
#endif

    if (pool) {
        cprng = apr_pcalloc(pool, sizeof(*cprng));
    }
    else {
        cprng = calloc(1, sizeof(*cprng));
    }
    if (!cprng) {
        return APR_ENOMEM;
    }
    cprng->flags = flags;
    cprng->pool = pool;

    if (bufsize == 0) {
        bufsize = CPRNG_BUF_SIZE_DEF;
    }
    else if (bufsize < CPRNG_BUF_SIZE_MIN) {
        bufsize = CPRNG_BUF_SIZE_MIN;
    }
    else if (bufsize % CPRNG_KEY_SIZE) {
        bufsize += CPRNG_KEY_SIZE;
        bufsize -= bufsize % CPRNG_KEY_SIZE;
    }
    if (pool) {
        cprng->key = apr_palloc(pool, CPRNG_KEY_SIZE + bufsize);
    }
    else {
        cprng->key = malloc(CPRNG_KEY_SIZE + bufsize);
    }
    if (!cprng->key) {
        cprng_cleanup(cprng);
        return APR_ENOMEM;
    }
    cprng->buf = cprng->key + CPRNG_KEY_SIZE;
    cprng->len = bufsize;

    rv = cprng_stream_ctx_make(&cprng->ctx);
    if (rv != APR_SUCCESS) {
        cprng_cleanup(cprng);
        return rv;
    }

    if (seed) {
        memset(cprng->key, 0, CPRNG_KEY_SIZE);
    }
    rv = apr_crypto_prng_reseed(cprng, seed);
    if (rv != APR_SUCCESS) {
        cprng_cleanup(cprng);
        return rv;
    }

#if APR_HAS_THREADS
    if (flags & APR_CRYPTO_PRNG_LOCKED) {
        rv = apr_thread_mutex_create(&cprng->mutex, APR_THREAD_MUTEX_DEFAULT,
                                     pool);
        if (rv != APR_SUCCESS) {
            cprng_cleanup(cprng);
            return rv;
        }
    }
#endif

    if (cprng_global && !(flags & APR_CRYPTO_PRNG_PER_THREAD)) {
        cprng_ring_lock();
        APR_RING_INSERT_TAIL(cprng_ring, cprng, apr_crypto_prng_t, link);
        cprng_ring_unlock();
    }
    else {
        APR_RING_ELEM_INIT(cprng, link);
    }

    if (pool) {
        apr_pool_cleanup_register(pool, cprng, cprng_cleanup,
                                  apr_pool_cleanup_null);
    }

    *pcprng = cprng;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_destroy(apr_crypto_prng_t *cprng)
{
    if (!cprng->pool) {
        return cprng_cleanup(cprng);
    }

    return apr_pool_cleanup_run(cprng->pool, cprng, cprng_cleanup);
}

static apr_status_t cprng_stream_bytes(apr_crypto_prng_t *cprng,
                                       unsigned char *key, void *to,
                                       size_t len)
{
    apr_status_t rv;

    rv = cprng_stream_ctx_bytes(&cprng->ctx, key, to, len, cprng->buf);
    if (rv != APR_SUCCESS && len) {
        apr_crypto_memzero(to, len);
    }
    return rv;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_reseed(apr_crypto_prng_t *cprng,
                                                 const unsigned char seed[])
{
    apr_status_t rv = APR_SUCCESS;

    if (!cprng) {
        /* Fall through with global CPRNG. */
        cprng = cprng_global;
        if (!cprng) {
            return APR_EINIT;
        }
    }

    cprng_lock(cprng);

    cprng->pos = cprng->len;
    apr_crypto_memzero(cprng->buf, cprng->len);
    if (seed) {
        apr_size_t n = 0;
        do {
            cprng->key[n] ^= seed[n];
        } while (++n < CPRNG_KEY_SIZE);
    }
    else if (cprng_global && cprng_global != cprng) {
        /* Use the global CPRNG: no need for more than the initial entropy. */
        rv = apr_crypto_random_bytes(cprng->key, CPRNG_KEY_SIZE);
    }
    else {
        /* Use the system entropy, i.e. one of "/dev/[u]random", getrandom(),
         * arc4random()... This may block but still we really want to wait for
         * the system to gather enough entropy for these 32 initial bytes, much
         * more than we want non-random bytes, and that's once and for all! 
         */
        rv = apr_generate_random_bytes(cprng->key, CPRNG_KEY_SIZE);
    }
    if (rv == APR_SUCCESS) {
        /* Init/set the stream with the new key, without buffering for now
         * so that the buffer and/or the next random bytes won't be generated
         * directly from this key but from the first stream bytes it generates,
         * i.e. the next key is always extracted from the stream cipher state
         * and cleared upon use.
         */
        rv = cprng_stream_bytes(cprng, cprng->key, NULL, 0);
    }

    cprng_unlock(cprng);

    return rv;
}

static apr_status_t cprng_bytes(apr_crypto_prng_t *cprng,
                                void *buf, apr_size_t len)
{
    unsigned char *ptr = buf;
    apr_status_t rv;
    apr_size_t n;

    while (len) {
        n = cprng->len - cprng->pos;
        if (n == 0) {
            n = cprng->len;
            if (len >= n) {
                do {
                    rv = cprng_stream_bytes(cprng, cprng->key, ptr, n);
                    if (rv != APR_SUCCESS) {
                        return rv;
                    }
                    ptr += n;
                    len -= n;
                } while (len >= n);
                if (!len) {
                    break;
                }
            }
            rv = cprng_stream_bytes(cprng, cprng->key, cprng->buf, n);
            if (rv != APR_SUCCESS) {
                return rv;
            }
            cprng->pos = 0;
        }
        if (n > len) {
            n = len;
        }

        /* Random bytes are consumed then zero-ed to ensure
         * both forward secrecy and cleared next mixed data.
         */
        memcpy(ptr, cprng->buf + cprng->pos, n);
        apr_crypto_memzero(cprng->buf + cprng->pos, n);
        cprng->pos += n;

        ptr += n;
        len -= n;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_bytes(apr_crypto_prng_t *cprng,
                                                void *buf, apr_size_t len)
{
    apr_status_t rv;

    if (!cprng) {
        /* Fall through with global CPRNG. */
        cprng = cprng_global;
        if (!cprng) {
            return APR_EINIT;
        }
    }

    cprng_lock(cprng);

    rv = cprng_bytes(cprng, buf, len);

    cprng_unlock(cprng);

    return rv;
}

/* Reset the buffer and use the next stream bytes as the new key. */
static apr_status_t cprng_newkey(apr_crypto_prng_t *cprng)
{
    cprng->pos = cprng->len;
    apr_crypto_memzero(cprng->buf, cprng->len);
    return cprng_stream_bytes(cprng, NULL, cprng->key, CPRNG_KEY_SIZE);
}

APR_DECLARE(apr_status_t) apr_crypto_prng_rekey(apr_crypto_prng_t *cprng)
{
    apr_status_t rv;

    if (!cprng) {
        /* Fall through with global CPRNG. */
        cprng = cprng_global;
        if (!cprng) {
            return APR_EINIT;
        }
    }

    cprng_lock(cprng);

    /* Renew and apply the new key. */
    rv = cprng_newkey(cprng);
    if (rv == APR_SUCCESS) {
        rv = cprng_stream_bytes(cprng, cprng->key, NULL, 0);
    }

    cprng_unlock(cprng);

    if (cprng == cprng_global) {
        /* Forward to all maintained CPRNGs. */
        cprng_ring_lock();
        for (cprng = APR_RING_FIRST(cprng_ring);
             cprng != APR_RING_SENTINEL(cprng_ring, apr_crypto_prng_t, link);
             cprng = APR_RING_NEXT(cprng, link)) {
            apr_status_t rt = apr_crypto_prng_rekey(cprng);
            if (rt != APR_SUCCESS && rv == APR_SUCCESS) {
                rv = rt;
            }
        }
        cprng_ring_unlock();
    }

    return rv;
}

#if APR_HAS_FORK
APR_DECLARE(apr_status_t) apr_crypto_prng_after_fork(apr_crypto_prng_t *cprng,
                                                     int in_child)
{
    apr_status_t rv;

    if (!cprng) {
        /* Fall through with global CPRNG. */
        cprng = cprng_global;
        if (!cprng) {
            return APR_EINIT;
        }
    }

    cprng_lock(cprng);

    /* Make sure the parent and child processes never share the same state, so
     * renew the key first (also clears the buffer) for both parent and child,
     * so that further fork()s (from parent or child) won't use the same state.
     */
    rv = cprng_newkey(cprng);
    if (rv == APR_SUCCESS && !in_child) {
        /* For the parent process only, renew a second time to ensure that key
         * material is different from the child.
         */
        rv = cprng_stream_bytes(cprng, NULL, cprng->key, CPRNG_KEY_SIZE);
    }
    if (rv == APR_SUCCESS) {
        /* Finally apply the new key, parent and child ones will now be
         * different and unknown to each other (including at the stream ctx
         * level).
         */
        rv = cprng_stream_bytes(cprng, cprng->key, NULL, 0);
    }

    cprng_unlock(cprng);

    if (cprng == cprng_global) {
        /* Forward to all maintained CPRNGs. */
        cprng_ring_lock();
        for (cprng = APR_RING_FIRST(cprng_ring);
             cprng != APR_RING_SENTINEL(cprng_ring, apr_crypto_prng_t, link);
             cprng = APR_RING_NEXT(cprng, link)) {
            apr_status_t rt = apr_crypto_prng_after_fork(cprng, in_child);
            if (rt != APR_SUCCESS && rv == APR_SUCCESS) {
                rv = rt;
            }
        }
        cprng_ring_unlock();
    }

    return rv;
}
#endif /* APR_HAS_FORK */

#endif /* APU_HAVE_CRYPTO_PRNG */
#endif /* APU_HAVE_CRYPTO */
