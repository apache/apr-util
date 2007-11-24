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

#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_strings.h"
#define APR_WANT_MEMFUNC
#define APR_WANT_STRFUNC
#include "apr_want.h"
#include "apr_general.h"

#include "apu_config.h"

#ifdef APU_HAVE_OPENSSL

#include "apu.h"
#include "apr_portable.h"


#include "apr_ssl.h"
#include "apr_ssl_private.h"
#include "apr_ssl_openssl_private.h"

APU_DECLARE(apr_status_t) apu_ssl_init(void)
{
    CRYPTO_malloc_init();
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    return APR_SUCCESS;
}

/* SSL_get_error() docs say that this MUST be called in the same
 * thread as the operation that failed, and that no other
 * SSL_ operations should be called between the error being reported
 * and the call to get the error code made, hence this function should
 * be called within the function that generates the error.
 * TODO - this should be expanded to generate the correct APR_ errors
 *        when we have created the mappings :-)
 */
static void openssl_get_error(apr_ssl_socket_t * sock, int fncode)
{
    sock->sslData->err = fncode;
    sock->sslData->sslErr = SSL_get_error(sock->sslData->ssl, fncode);
}

/* The apr_ssl_factory_t structure will have the pool and purpose
 * fields set only.
 */
APU_DECLARE(apr_status_t) apu_ssl_factory_create(apr_ssl_factory_t * asf,
                                                 const char *privateKeyFn,
                                                 const char *certFn,
                                                 const char *digestType)
{
    apu_ssl_data_t *sslData = apr_pcalloc(asf->pool, sizeof(*sslData));
    if (!sslData) {
        return APR_ENOMEM;
    }

    if (asf->purpose == APR_SSL_FACTORY_SERVER) {
        sslData->ctx = SSL_CTX_new(SSLv23_server_method());
        if (sslData->ctx) {
            if (!SSL_CTX_use_PrivateKey_file(sslData->ctx, privateKeyFn,
                                             SSL_FILETYPE_PEM) ||
                !SSL_CTX_use_certificate_file(sslData->ctx, certFn,
                                              SSL_FILETYPE_PEM) ||
                !SSL_CTX_check_private_key(sslData->ctx)) {
                SSL_CTX_free(sslData->ctx);
                return APR_ENOENT;        /* what code should we return? */
            }
        }
    }
    else {
        sslData->ctx = SSL_CTX_new(SSLv23_client_method());
    }

    if (digestType) {
        sslData->md = EVP_get_digestbyname(digestType);
        /* we don't care if this fails... */
    }

    if (!sslData->ctx)
        return APR_EGENERAL;        /* what error code? */

    asf->sslData = sslData;

    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apu_ssl_socket_create(apr_ssl_socket_t * sslSock,
                                                apr_ssl_factory_t * asf)
{
    apu_ssl_socket_data_t *sslData = apr_pcalloc(sslSock->pool,
                                                 sizeof(*sslData));
    apr_os_sock_t fd;

    if (!sslData) {
        return APR_ENOMEM;
    }
    if (!sslData || !asf->sslData) {
        return APR_EINVAL;
    }
    sslData->ssl = SSL_new(asf->sslData->ctx);
    if (!sslData->ssl) {
        return APR_EINVALSOCK;        /* Hmm, better error code? */
    }

    /*
     * Joe Orton points out this is actually wrong and assumes that that
     * we're on an "fd" system. We need some better way of handling this for
     * systems that don't use fd's for sockets. Will?
     */
    if (apr_os_sock_get(&fd, sslSock->plain) != APR_SUCCESS)
        return APR_EINVALSOCK;

    SSL_set_fd(sslData->ssl, fd);
    sslSock->sslData = sslData;
    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apu_ssl_socket_close(apr_ssl_socket_t * sock)
{
    int sslRv;

    if (!sock->sslData->ssl)
        return APR_SUCCESS;

    if (sock->connected) {
        if ((sslRv = SSL_shutdown(sock->sslData->ssl)) == 0)
            sslRv = SSL_shutdown(sock->sslData->ssl);
        if (sslRv == -1)
            return APR_EINVALSOCK;        /* Better error code to return? */
    }
    SSL_free(sock->sslData->ssl);
    sock->sslData->ssl = NULL;
    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apu_ssl_connect(apr_ssl_socket_t * sock)
{
    int sslOp;

    if (!sock->sslData->ssl)
        return APR_EINVAL;

    if ((sslOp = SSL_connect(sock->sslData->ssl)) == 1) {
        sock->connected = 1;
        return APR_SUCCESS;
    }
    openssl_get_error(sock, sslOp);
    return APR_EGENERAL;
}

APU_DECLARE(apr_status_t) apu_ssl_send(apr_ssl_socket_t * sock, const char *buf,
                                       apr_size_t * len)
{
    int sslOp;

    sslOp = SSL_write(sock->sslData->ssl, buf, *len);
    if (sslOp > 0) {
        *len = sslOp;
        return APR_SUCCESS;
    }
    openssl_get_error(sock, sslOp);
    return APR_EGENERAL;        /* SSL error? */
}

APU_DECLARE(apr_status_t) apu_ssl_recv(apr_ssl_socket_t * sock,
                                       char *buf, apr_size_t * len)
{
    int sslOp;

    if (!sock->sslData)
        return APR_EINVAL;

    sslOp = SSL_read(sock->sslData->ssl, buf, *len);
    if (sslOp > 0) {
        *len = sslOp;
        return APR_SUCCESS;
    }
    openssl_get_error(sock, sslOp);
    return APR_EGENERAL;        /* SSL error ? */
}

APU_DECLARE(apr_status_t) apu_ssl_accept(apr_ssl_socket_t * newSock,
                                         apr_ssl_socket_t * oldSock,
                                         apr_pool_t * pool)
{
    apu_ssl_socket_data_t *sslData = apr_pcalloc(pool, sizeof(*sslData));
    apr_os_sock_t fd;
    int sslOp;

    if (!sslData) {
        return APR_ENOMEM;
    }
    if (!oldSock->factory) {
        return APR_EINVAL;
    }

    sslData->ssl = SSL_new(oldSock->factory->sslData->ctx);
    if (!sslData->ssl) {
        return APR_EINVAL;
    }

    if (apr_os_sock_get(&fd, newSock->plain) != APR_SUCCESS) {
        return APR_EINVALSOCK;
    }
    SSL_set_fd(sslData->ssl, fd);

    newSock->pool = pool;
    newSock->sslData = sslData;
    newSock->factory = oldSock->factory;

    if ((sslOp = SSL_accept(sslData->ssl)) != 1) {
        openssl_get_error(newSock, sslOp);
        return APR_EGENERAL;
    }

    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apu_ssl_raw_error(apr_ssl_socket_t * sock)
{
    if (!sock->sslData)
        return APR_EINVAL;

    if (sock->sslData->sslErr)
        return sock->sslData->sslErr;

    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apr_evp_crypt_cleanup(apr_evp_crypt_t * e)
{

#if HAVE_DECL_EVP_PKEY_CTX_NEW
    if (e->pkeyCtx) {
        EVP_PKEY_CTX_free(e->pkeyCtx);
        e->pkeyCtx = NULL;
    }
#endif
    if (e->cipherCtx) {
        EVP_CIPHER_CTX_cleanup(e->cipherCtx);
        e->cipherCtx = NULL;
    }

    return APR_SUCCESS;

}

apr_status_t apr_evp_crypt_cleanup_helper(void *data)
{
    apr_evp_crypt_t *f = (apr_evp_crypt_t *) data;
    return apr_evp_crypt_cleanup(f);
}

APU_DECLARE(apr_status_t) apr_evp_factory_cleanup(apr_evp_factory_t * f)
{
    apu_evp_data_t *evpData = f->evpData;
    int i;

    for (i = 0; i < EVP_MAX_KEY_LENGTH; evpData->key[i++] = 0);
    for (i = 0; i < EVP_MAX_IV_LENGTH; evpData->iv[i++] = 0);
#if HAVE_DECL_EVP_PKEY_CTX_NEW
    if (evpData->ssl) {
        SSL_free(evpData->ssl);
        evpData->ssl = NULL;
    }
    if (evpData->sslCtx) {
        SSL_CTX_free(evpData->sslCtx);
        evpData->sslCtx = NULL;
    }
#endif

    return APR_SUCCESS;

}

apr_status_t apr_evp_factory_cleanup_helper(void *data)
{
    apr_evp_factory_t *f = (apr_evp_factory_t *) data;
    return apr_evp_factory_cleanup(f);
}

APU_DECLARE(apr_status_t) apr_evp_init(void)
{
    return apr_ssl_init();
}

APU_DECLARE(apr_status_t) apr_evp_factory_create(apr_evp_factory_t ** newFactory,
                                                 const char *privateKeyFn,
                                                 const char *certFn,
                                                 const char *cipherName,
                                                 const char *passphrase,
                                                 const char *engine,
                                                 const char *digest,
                                                 apr_evp_factory_type_e purpose,
                                                 apr_pool_t * pool)
{
    apr_evp_factory_t *f = apr_pcalloc(pool, sizeof(apr_evp_factory_t));
    apu_evp_data_t *data;
    if (!f) {
        return APR_ENOMEM;
    }
    *newFactory = f;
    f->pool = pool;
    f->purpose = purpose;

    data = apr_pcalloc(pool, sizeof(apu_evp_data_t));
    if (!data) {
        return APR_ENOMEM;
    }
    f->evpData = data;
    apr_pool_cleanup_register(pool, f,
                              apr_evp_factory_cleanup_helper,
                              apr_pool_cleanup_null);

    switch (purpose) {
    case APR_EVP_FACTORY_ASYM:{
#if HAVE_DECL_EVP_PKEY_CTX_NEW
            /* load certs */
            data->sslCtx = SSL_CTX_new(SSLv23_server_method());
            if (data->sslCtx) {
                if (!SSL_CTX_use_PrivateKey_file(data->sslCtx, privateKeyFn,
                                                 SSL_FILETYPE_PEM) ||
                    !SSL_CTX_use_certificate_file(data->sslCtx, certFn,
                                                  SSL_FILETYPE_PEM) ||
                    !SSL_CTX_check_private_key(data->sslCtx)) {
                    SSL_CTX_free(data->sslCtx);
                    return APR_ENOCERT;
                }
                data->ssl = SSL_new(data->sslCtx);
                if (data->ssl) {
                    X509 *cert;
                    data->privkey = SSL_get_privatekey(data->ssl);
                    cert = SSL_get_certificate(data->ssl);
                    if (cert) {
                        data->pubkey = X509_get_pubkey(cert);
                    }
                }
            }
#else
            return APR_ENOTIMPL;
#endif
        }
    case APR_EVP_FACTORY_SYM:{
            data->cipher = EVP_get_cipherbyname(cipherName);
            if (!data->cipher) {
                return APR_ENOCIPHER;
            }
            data->md = EVP_get_digestbyname(digest);
            if (!data->md) {
                return APR_ENODIGEST;
            }
            EVP_BytesToKey(data->cipher, data->md,
                           data->salt,
                  (const unsigned char *) passphrase, strlen(passphrase), 1,
                           data->key, data->iv);
        }
    }

    return APR_SUCCESS;

}

APU_DECLARE(apr_status_t) apr_evp_crypt_init(apr_evp_factory_t * f,
                                             apr_evp_crypt_t ** e,
                                             apr_evp_crypt_type_e type,
                                             apr_evp_crypt_key_e key,
                                             apr_pool_t * p)
{
    apu_evp_data_t *data = f->evpData;

    if (!*e) {
        *e = apr_pcalloc(p, sizeof(apr_evp_crypt_t));
    }
    if (!*e) {
        return APR_ENOMEM;
    }
    (*e)->pool = p;
    (*e)->purpose = f->purpose;
    (*e)->type = type;
    (*e)->key = key;

    switch (f->purpose) {
    case APR_EVP_FACTORY_ASYM:{
#if HAVE_DECL_EVP_PKEY_CTX_NEW

            /* todo: add ENGINE support */
            if (APR_EVP_KEY_PUBLIC == type) {
                (*e)->pkeyCtx = EVP_PKEY_CTX_new(data->pubkey, NULL);
            }
            else if (APR_EVP_KEY_PRIVATE == type) {
                (*e)->pkeyCtx = EVP_PKEY_CTX_new(data->privkey, NULL);
            }
            apr_pool_cleanup_register(p, *e, apr_evp_crypt_cleanup_helper,
                                      apr_pool_cleanup_null);

            if (APR_EVP_ENCRYPT == type) {
                if (EVP_PKEY_encrypt_init((*e)->pkeyCtx) <= 0) {
                    return APR_EINIT;
                }
            }
            else if (APR_EVP_DECRYPT == type) {
                if (EVP_PKEY_decrypt_init((*e)->pkeyCtx) <= 0) {
                    return APR_EINIT;
                }
            }

            return APR_SUCCESS;

#else
            return APR_ENOTIMPL;
#endif
        }
    case APR_EVP_FACTORY_SYM:{
            if (!(*e)->cipherCtx) {
                (*e)->cipherCtx = apr_pcalloc(p, sizeof(EVP_CIPHER_CTX));
                if (!(*e)->cipherCtx) {
                    return APR_ENOMEM;
                }
            }
            EVP_CIPHER_CTX_init((*e)->cipherCtx);
            EVP_CipherInit_ex((*e)->cipherCtx, data->cipher, NULL, data->key, data->iv, type);
            return APR_SUCCESS;
        }
    }

    return APR_EINIT;

}

APU_DECLARE(apr_status_t) apr_evp_crypt(apr_evp_crypt_t * e,
                                        unsigned char **out,
                                        apr_size_t * outlen,
                                        const unsigned char *in,
                                        apr_size_t inlen)
{
    unsigned char *buffer;

    switch (e->purpose) {
    case APR_EVP_FACTORY_ASYM:{
#if HAVE_DECL_EVP_PKEY_CTX_NEW
            if (!out || !*out) {
                if (APR_EVP_ENCRYPT == e->type &&
                    EVP_PKEY_encrypt(e->pkeyCtx, NULL, outlen,
                                     in, inlen) <= 0) {
                    return APR_EGENERAL;
                }
                if (APR_EVP_DECRYPT == e->type &&
                    EVP_PKEY_decrypt(e->pkeyCtx, NULL, outlen,
                                     in, inlen) <= 0) {
                    return APR_EGENERAL;
                }
                if (!out) {
                    return APR_SUCCESS;
                }
                buffer = apr_palloc(e->pool, *outlen + 1);
                if (!buffer) {
                    return APR_ENOMEM;
                }
                *out = buffer;
                buffer[*outlen] = 0;
            }
            if (APR_EVP_ENCRYPT == e->type &&
                EVP_PKEY_encrypt(e->pkeyCtx, *out, outlen,
                                 in, inlen) <= 0) {
                return APR_EGENERAL;
            }
            if (APR_EVP_DECRYPT == e->type &&
                EVP_PKEY_decrypt(e->pkeyCtx, *out, outlen,
                                 in, inlen) <= 0) {
                return APR_EGENERAL;
            }

            return APR_SUCCESS;

#else
            return APR_ENOTIMPL;
#endif
        }
    case APR_EVP_FACTORY_SYM:{
            int len = (int) *outlen;
            if (!out) {
                *outlen = inlen + EVP_MAX_BLOCK_LENGTH;
                return APR_SUCCESS;
            }
            if (!*out) {
                buffer = apr_palloc(e->pool, inlen + EVP_MAX_BLOCK_LENGTH);
                if (!buffer) {
                    return APR_ENOMEM;
                }
                *out = buffer;
            }
            if (!EVP_CipherUpdate(e->cipherCtx, *out, &len, in, inlen)) {
                return APR_EGENERAL;
            }
            *outlen = (apr_size_t) len;
            return APR_SUCCESS;
        }
    }

    return APR_EGENERAL;

}

APU_DECLARE(apr_status_t) apr_evp_crypt_finish(apr_evp_crypt_t * e,
                                               unsigned char *out,
                                               apr_size_t * outlen)
{

    switch (e->purpose) {
    case APR_EVP_FACTORY_ASYM:{
#if HAVE_DECL_EVP_PKEY_CTX_NEW
            break;
#else
            return APR_ENOTIMPL;
#endif
        }
    case APR_EVP_FACTORY_SYM:{
            int tlen;
            if (!EVP_CipherFinal_ex(e->cipherCtx, out, &tlen)) {
                return APR_EGENERAL;
            }
            *outlen = tlen;
            break;
        }
    }
    apr_evp_crypt_cleanup(e);

    return APR_SUCCESS;

}

#endif
