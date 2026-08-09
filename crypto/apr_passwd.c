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

#include "apr_version.h"
#include "apr_strings.h"
#include "apr_md5.h"
#include "apr_lib.h"
#include "apr_sha1.h"
#include "apu_config.h"
#include "crypt_blowfish.h"

#if APR_HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_CRYPT_H
#include <crypt.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#if APR_HAVE_PTHREAD_H
#include <pthread.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif

static const char * const apr1_id = "$apr1$";

#if APR_VERSION_AT_LEAST(1,8,0)

#define streq_timingsafe    apr_streq_timingsafe
#define strneq_timingsafe   apr_strneq_timingsafe

#else /* borrow code from APR-1.8 if not available */

/* A volatile variable which is always zero but allows to block the compiler
 * from optimizing or eliding code using it. Volatile forces the compiler to
 * emit a memory load for which no value can be assumed, so for instance an
 * add/sub/xor/or with "optblocker" is a noop that will hide the result to
 * the optimizer.
 */
static volatile const apr_uint32_t optblocker;

/* Return whether x is not zero, with no branching controlled by x.
 *
 * Taken from the cryptoint library (public domain) by D. J. Bernstein,
 * which provides timing attacks safe integer operations/primitives.
 * Code:
 *   https://lib.mceliece.org/libmceliece-20250507/cryptoint/crypto_uint32.h
 * Paper:
 *   https://cr.yp.to/papers/cryptoint-20250424.pdf
 */
#ifndef __has_attribute
#define __has_attribute(__x)    0
#endif

#if __has_attribute(always_inline)
__attribute__((always_inline))
#endif
static APR_INLINE int test_nonzero_timingsafe(apr_uint32_t x)
{
    x |= -x; /* sets the most significant bit unless x == 0 */

    /* shift bit 31 (MSB) to bit 0 */
    x >>= 32-6;      /* keep 6 bits */
    x += optblocker; /* lose the optimizer */
    x >>= 5;         /* keep the (original) MSB only */

    /* x is now 0 or 1 */
    return x & INT_MAX;
}

static int streq_timingsafe(const char *sec1, const char *str2)
{
    apr_uint32_t diff = 0;
    apr_size_t i1 = 0, i2 = 0;

    for (;; ++i2) {
        const unsigned char c1 = ((volatile const unsigned char *)sec1)[i1];
        const unsigned char c2 = ((volatile const unsigned char *)str2)[i2];

        diff |= c1 ^ c2; /* sets diff to non-zero whenever c1 != c2 */

        /* Not a shortest/longest match because an attacker would usually know
         * one of the strings and could then determine the length of the other.
         * So assume only sec1 and its length are secret and stop the loop at
         * the end of str2. If sec1 is shorter than str2 the loop will continue
         * by comparing the rest of str2 with the trailing NUL byte of sec1.
         * In any case since the diff above is computed up to and including a
         * NUL byte, only the same content and length will raise match.
         */
        if (!c2) {
            break;
        }

        /* Don't go above sec1's NUL byte */
        i1 += test_nonzero_timingsafe(c1);
    }

    /* (diff == 0) <=> (diff != 0) ^ 1 */
    return test_nonzero_timingsafe(diff) ^ 1;
}

static int strneq_timingsafe(const char *sec1, const char *str2, apr_size_t n)
{
    apr_uint32_t diff = 0;
    volatile apr_size_t count = n; /* prevent loop unrolling */
    apr_size_t i1 = 0, i2 = 0;

    for (; i2 < count; ++i2) {
        const unsigned char c1 = ((volatile const unsigned char *)sec1)[i1];
        const unsigned char c2 = ((volatile const unsigned char *)str2)[i2];

        diff |= c1 ^ c2; /* sets diff to non-zero whenever c1 != c2 */

        /* Not a shortest/longest match because an attacker would usually know
         * one of the strings and could then determine the length of the other.
         * So assume only sec1 and its length are secret and stop the loop at
         * the end of str2. If sec1 is shorter than str2 the loop will continue
         * by comparing the rest of str2 with the trailing NUL byte of sec1.
         * In any case since the diff above is computed up to and including a
         * NUL byte, only the same content and length will raise match.
         */
        if (!c2) {
            break;
        }

        /* Don't go above sec1's NUL byte */
        i1 += test_nonzero_timingsafe(c1);
    }

    /* (diff == 0) <=> (diff != 0) ^ 1 */
    return test_nonzero_timingsafe(diff) ^ 1;
}

#endif /* APR_VERSION_AT_LEAST(1,8,0) */

#if !defined(WIN32) && !defined(BEOS) && !defined(NETWARE)
#if defined(APU_CRYPT_THREADSAFE) || !APR_HAS_THREADS || \
    defined(CRYPT_R_CRYPTD) || defined(CRYPT_R_STRUCT_CRYPT_DATA)

#define crypt_mutex_lock()
#define crypt_mutex_unlock()

#elif APR_HAVE_PTHREAD_H && defined(PTHREAD_MUTEX_INITIALIZER)

static pthread_mutex_t crypt_mutex = PTHREAD_MUTEX_INITIALIZER;
static void crypt_mutex_lock(void)
{
    pthread_mutex_lock(&crypt_mutex);
}

static void crypt_mutex_unlock(void)
{
    pthread_mutex_unlock(&crypt_mutex);
}

#else

#error apr_password_validate() is not threadsafe.  rebuild APR without thread support.

#endif
#endif

#if defined(WIN32) || defined(BEOS) || defined(NETWARE) || defined(__ANDROID__)
#define CRYPT_MISSING 1
#else
#define CRYPT_MISSING 0
#endif

/*
 * Validate a plaintext password against a smashed one.  Uses either
 * crypt() (if available) or apr_md5_encode() or apr_sha1_base64(), depending
 * upon the format of the smashed input password.  Returns APR_SUCCESS if
 * they match, or APR_EMISMATCH if they don't.  If the platform doesn't
 * support crypt, then the default check is against a clear text string.
 */
APU_DECLARE(apr_status_t) apr_password_validate(const char *passwd, 
                                                const char *hash)
{
    char sample[200];
#if !CRYPT_MISSING
    char *crypt_pw;
#endif

    if ((strneq_timingsafe(hash, "$2a$", 4) | /* test both */
         strneq_timingsafe(hash, "$2y$", 4))) {
        /*
         * The hash was created using [apr_]bcrypt encoding.
         */
        if (_crypt_blowfish_rn(passwd, hash, sample, sizeof(sample)) == NULL)
            return APR_FROM_OS_ERROR(errno);
    }
    else if (strneq_timingsafe(hash, apr1_id, strlen(apr1_id))) {
        /*
         * The hash was created using our custom algorithm.
         */
        apr_md5_encode(passwd, hash, sample, sizeof(sample));
    }
    else if (strneq_timingsafe(hash, APR_SHA1PW_ID, APR_SHA1PW_IDLEN)) {
        /*
         * The hash is a (naked) SHA1.
         */
        apr_sha1_base64(passwd, (int)strlen(passwd), sample);
    }
    else {
        /*
         * It's not our algorithm, so feed it to crypt() if possible.
         */
#if CRYPT_MISSING
        return streq_timingsafe(hash, passwd) ? APR_SUCCESS : APR_EMISMATCH;
#elif defined(CRYPT_R_CRYPTD)
        apr_status_t rv;
        CRYPTD *buffer = malloc(sizeof(*buffer));

        if (buffer == NULL)
            return APR_ENOMEM;
        crypt_pw = crypt_r(passwd, hash, buffer);
        if (!crypt_pw)
            rv = APR_EMISMATCH;
        else
            rv = streq_timingsafe(hash, crypt_pw) ? APR_SUCCESS : APR_EMISMATCH;
        free(buffer);
        return rv;
#elif defined(CRYPT_R_STRUCT_CRYPT_DATA)
        apr_status_t rv;
        struct crypt_data *buffer = malloc(sizeof(*buffer));

        if (buffer == NULL)
            return APR_ENOMEM;

#ifdef __GLIBC_PREREQ
        /*
         * For not too old glibc (>= 2.3.2), it's enough to set
         * buffer.initialized = 0. For < 2.3.2 and for other platforms,
         * we need to zero the whole struct.
         */
#if __GLIBC_PREREQ(2,4)
#define USE_CRYPT_DATA_INITALIZED
#endif
#endif

#ifdef USE_CRYPT_DATA_INITALIZED
        buffer->initialized = 0;
#else
        memset(buffer, 0, sizeof(*buffer));
#endif

        crypt_pw = crypt_r(passwd, hash, buffer);
        if (!crypt_pw)
            rv = APR_EMISMATCH;
        else
            rv = streq_timingsafe(hash, crypt_pw) ? APR_SUCCESS : APR_EMISMATCH;
        free(buffer);
        return rv;
#else
        /* Do a bit of sanity checking since we know that crypt_r()
         * should always be used for threaded builds on AIX, and
         * problems in configure logic can result in the wrong
         * choice being made.
         */
#if defined(_AIX) && APR_HAS_THREADS
#error Configuration error!  crypt_r() should have been selected!
#endif
        {
            apr_status_t rv;

            /* Handle thread safety issues by holding a mutex around the
             * call to crypt().
             */
            crypt_mutex_lock();
            crypt_pw = crypt(passwd, hash);
            if (!crypt_pw) {
                rv = APR_EMISMATCH;
            }
            else {
                rv = streq_timingsafe(hash, crypt_pw) ? APR_SUCCESS : APR_EMISMATCH;
            }
            crypt_mutex_unlock();
            return rv;
        }
#endif
    }
    return streq_timingsafe(hash, sample) ? APR_SUCCESS : APR_EMISMATCH;
}

static const char * const bcrypt_id = "$2y$";
APU_DECLARE(apr_status_t) apr_bcrypt_encode(const char *pw,
                                            unsigned int count,
                                            const unsigned char *salt,
                                            apr_size_t salt_len,
                                            char *out, apr_size_t out_len)
{
    char setting[40];
    if (_crypt_gensalt_blowfish_rn(bcrypt_id, count, (const char *)salt,
                                   salt_len, setting, sizeof(setting)) == NULL)
        return APR_FROM_OS_ERROR(errno);
    if (_crypt_blowfish_rn(pw, setting, out, out_len) == NULL)
        return APR_FROM_OS_ERROR(errno);
    return APR_SUCCESS;
}
