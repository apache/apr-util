/* Copyright 2000-2006 The Apache Software Foundation or its licensors, as
 * applicable.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
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

apr_status_t _ssl_init(void)
{
    CRYPTO_malloc_init();
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    return APR_SUCCESS;
}

apr_status_t _ssl_factory_create(apr_ssl_factory_t *asf,
                                 const char *privateKeyFn,
                                 const char *certFn,
                                 const char *digestType)
{
    _apu_ssl_data_t *sslData = apr_pcalloc(asf->pool, sizeof(*sslData));
    if (!sslData) {
        return -1;
    }

    if (privateKeyFn && certFn) {
        sslData->ctx = SSL_CTX_new(SSLv23_server_method());
        if (sslData->ctx) {
            if (!SSL_CTX_use_PrivateKey_file(sslData->ctx, privateKeyFn, SSL_FILETYPE_PEM) ||
                !SSL_CTX_use_certificate_file(sslData->ctx, certFn, SSL_FILETYPE_PEM) ||
                !SSL_CTX_check_private_key(sslData->ctx)) {
                SSL_CTX_free(sslData->ctx);
                return -1; // code?
            }
        }
    } else {
        sslData->ctx = SSL_CTX_new(SSLv23_client_method());
    }

    if (digestType) {
        sslData->md = EVP_get_digestbyname(digestType);
        // we don't care if this fails...
    }

    if (!sslData->ctx)
        return APR_EGENERAL; // what code?

    asf->sslData = sslData;

    return APR_SUCCESS;
}

apr_status_t _ssl_socket_create(apr_ssl_socket_t *sslSock, apr_ssl_factory_t *asf)
{
    _apu_ssl_socket_data_t *sslData = apr_pcalloc(sslSock->pool, sizeof(*sslData));
    apr_os_sock_t fd;

    if (!sslData || !asf->sslData)
        return -1;
    sslData->ssl = SSL_new(asf->sslData->ctx);
    if (!sslData->ssl)
        return -1;

    if (apr_os_sock_get(&fd, sslSock->plain) != APR_SUCCESS)
        return -1;

    SSL_set_fd(sslData->ssl, fd);
    sslSock->sslData = sslData;
    return APR_SUCCESS;
}

apr_status_t _ssl_socket_close(apr_ssl_socket_t *sock)
{
    int sslRv;
    apr_status_t rv;

    if (!sock->sslData->ssl)
        return APR_SUCCESS;
    if (sock->connected) {
        if ((sslRv = SSL_shutdown(sock->sslData->ssl)) == 0)
            sslRv = SSL_shutdown(sock->sslData->ssl);
        if (sslRv == -1)
            return -1;
    }
    SSL_free(sock->sslData->ssl);
    sock->sslData->ssl = NULL;
    return APR_SUCCESS;
}

apr_status_t _ssl_connect(apr_ssl_socket_t *sock)
{
    if (!sock->sslData->ssl)
        return APR_EINVAL;

    if (SSL_connect(sock->sslData->ssl)) {
        sock->connected = 1;
        return APR_SUCCESS;
    }
    return -1;
}

apr_status_t _ssl_send(apr_ssl_socket_t *sock, const char *buf, apr_size_t *len)
{
    apr_status_t rv;
    int sslOp;

    sslOp = SSL_write(sock->sslData->ssl, buf, *len);
    if (sslOp > 0) {
        *len = sslOp;
        return APR_SUCCESS;
    }
    return -1;
}

apr_status_t _ssl_recv(apr_ssl_socket_t * sock,
                              char *buf, apr_size_t *len)
{
    int sslOp;

    sslOp = SSL_read(sock->sslData->ssl, buf, *len);
    if (sslOp > 0) {
        *len = sslOp;
        return APR_SUCCESS;
    }
    return -1;
}

apr_status_t _ssl_accept(apr_ssl_socket_t *newSock, apr_ssl_socket_t *oldSock, apr_pool_t *pool)
{
    _apu_ssl_socket_data_t *sslData = apr_pcalloc(pool, sizeof(*sslData));
    apr_os_sock_t fd;

    if (!sslData || !oldSock->factory)
        return -1;

    sslData->ssl = SSL_new(oldSock->factory->sslData->ctx);
    if (!sslData->ssl)
        return -1;

    if (apr_os_sock_get(&fd, newSock->plain) != APR_SUCCESS)
        return -1;
    SSL_set_fd(sslData->ssl, fd);

    newSock->pool = pool;
    newSock->sslData = sslData;
    newSock->factory = oldSock->factory;
    return APR_SUCCESS;
}

#endif
