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

#include "apu.h"
#include "apr_portable.h"

#ifdef APU_HAVE_WINSOCKSSL


#include "apr_ssl.h"
#include "apr_ssl_private.h"
#include "apr_ssl_winsock_private.h"

apr_status_t apu_ssl_init(void)
{
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
//static void openssl_get_error(apr_ssl_socket_t *sock, int fncode)
//{
//    sock->sslData->err = fncode;
//    sock->sslData->sslErr = SSL_get_error(sock->sslData->ssl, fncode);
//}

apr_status_t apu_ssl_factory_create(apr_ssl_factory_t *asf,
                                 const char *privateKeyFn,
                                 const char *certFn,
                                 const char *digestType)
{
    apu_ssl_data_t *sslData = apr_pcalloc(asf->pool, sizeof(*sslData));
    if (!sslData) {
        return -1;
    }

//    if (privateKeyFn && certFn) {
//        sslData->ctx = SSL_CTX_new(SSLv23_server_method());
//        if (sslData->ctx) {
//            if (!SSL_CTX_use_PrivateKey_file(sslData->ctx, privateKeyFn,
//                                             SSL_FILETYPE_PEM) ||
//                !SSL_CTX_use_certificate_file(sslData->ctx, certFn, 
//                                              SSL_FILETYPE_PEM) ||
//                !SSL_CTX_check_private_key(sslData->ctx)) {
//                SSL_CTX_free(sslData->ctx);
//                return APR_ENOENT; /* what code shoudl we return? */
//            }
//        }
//    } else {
//        sslData->ctx = SSL_CTX_new(SSLv23_client_method());
//    }
//
//    if (digestType) {
//        sslData->md = EVP_get_digestbyname(digestType);
//        /* we don't care if this fails... */
//    }
//
//    if (!sslData->ctx)
//        return APR_EGENERAL; /* what error code? */


    asf->sslData = sslData;

    return APR_SUCCESS;
}

apr_status_t apu_ssl_socket_create(apr_ssl_socket_t *sslSock, 
                                   apr_ssl_factory_t *asf)
{
    apu_ssl_socket_data_t *sslData = apr_pcalloc(sslSock->pool, 
                                                 sizeof(*sslData));
    apr_os_sock_t fd;
    struct tlsclientopts sWS2Opts;
    struct nwtlsopts sNWTLSOpts;
    unsigned long ulFlags;
    int rcode;
    struct sslserveropts opts;
    unicode_t keyFileName[60];

    if (!sslData || !asf->sslData)
        return -1;
//    sslData->ssl = SSL_new(asf->sslData->ctx);
//    if (!sslData->ssl)
//        return -1;
//
//    if (apr_os_sock_get(&fd, sslSock->plain) != APR_SUCCESS)
//        return -1;
//
//    SSL_set_fd(sslData->ssl, fd);
 
    apr_os_sock_get(&fd, sslSock->plain);

    /* zero out buffers */
    memset((char *)&sWS2Opts, 0, sizeof(struct tlsclientopts));
    memset((char *)&sNWTLSOpts, 0, sizeof(struct nwtlsopts));

    /* turn on ssl for the socket */
//    ulFlags = (numcerts ? SO_TLS_ENABLE : SO_TLS_ENABLE | SO_TLS_BLIND_ACCEPT);
    ulFlags = SO_TLS_ENABLE | SO_TLS_BLIND_ACCEPT;
    rcode = WSAIoctl(fd, SO_TLS_SET_FLAGS, &ulFlags, sizeof(unsigned long),
                 NULL, 0, NULL, NULL, NULL);
    if (SOCKET_ERROR == rcode)
    {
        return rcode;
    }

    ulFlags = SO_TLS_UNCLEAN_SHUTDOWN;
    WSAIoctl(fd, SO_TLS_SET_FLAGS, &ulFlags, sizeof(unsigned long),
                 NULL, 0, NULL, NULL, NULL);

    /* setup the socket for SSL */
    memset (&sWS2Opts, 0, sizeof(sWS2Opts));
    memset (&sNWTLSOpts, 0, sizeof(sNWTLSOpts));
    sWS2Opts.options = &sNWTLSOpts;

//    if (numcerts) {
//        sNWTLSOpts.walletProvider = WAL_PROV_DER;   //the wallet provider defined in wdefs.h
//        sNWTLSOpts.TrustedRootList = certarray;     //array of certs in UNICODE format
//        sNWTLSOpts.numElementsInTRList = numcerts;  //number of certs in TRList
//    }
//    else {
        /* setup the socket for SSL */
        unicpy(keyFileName, L"SSL CertificateIP");
        sWS2Opts.wallet = keyFileName;    /* no client certificate */
        sWS2Opts.walletlen = unilen(keyFileName);

        sNWTLSOpts.walletProvider = WAL_PROV_KMO;  //the wallet provider defined in wdefs.h
//    }

    /* make the IOCTL call */
    rcode = WSAIoctl(fd, SO_TLS_SET_CLIENT, &sWS2Opts,
                     sizeof(struct tlsclientopts), NULL, 0, NULL,
                     NULL, NULL);

    /* make sure that it was successfull */
    if(SOCKET_ERROR == rcode ) {
        return rcode;
    }
 
    sslSock->sslData = sslData;

    return APR_SUCCESS;
}

apr_status_t apu_ssl_socket_close(apr_ssl_socket_t *sock)
{
//    int sslRv;
//    apr_status_t rv;
//
//    if (!sock->sslData->ssl)
//        return APR_SUCCESS;
//    if (sock->connected) {
//        if ((sslRv = SSL_shutdown(sock->sslData->ssl)) == 0)
//            sslRv = SSL_shutdown(sock->sslData->ssl);
//        if (sslRv == -1)
//            return -1;
//    }
//    SSL_free(sock->sslData->ssl);
//    sock->sslData->ssl = NULL;
    return APR_SUCCESS;
}

apr_status_t apu_ssl_connect(apr_ssl_socket_t *sock)
{
//    int sslOp;
//
//    if (!sock->sslData->ssl)
//        return APR_EINVAL;
//
//    if ((sslOp = SSL_connect(sock->sslData->ssl)) == 1) {
//        sock->connected = 1;
//        return APR_SUCCESS;
//    }
//    openssl_get_error(sock, sslOp);
    return -1;
}

apr_status_t apu_ssl_send(apr_ssl_socket_t *sock, const char *buf, 
                          apr_size_t *len)
{
    return apr_socket_send(sock->plain, buf, len);
}

apr_status_t apu_ssl_recv(apr_ssl_socket_t *sock,
                              char *buf, apr_size_t *len)
{
    return apr_socket_recv(sock->plain, buf, len);
}

apr_status_t apu_ssl_accept(apr_ssl_socket_t *newSock, 
                            apr_ssl_socket_t *oldSock, apr_pool_t *pool)
{
    apu_ssl_socket_data_t *sslData = apr_pcalloc(pool, sizeof(*sslData));
    apr_status_t ret;

    ret = apr_socket_accept(&(newSock->plain), oldSock->plain, pool);

    if (ret == APR_SUCCESS) {
        newSock->pool = pool;
        newSock->sslData = sslData;
        newSock->factory = oldSock->factory;
    }

    return ret;
}

apr_status_t apu_ssl_raw_error(apr_ssl_socket_t *sock)
{
    if (!sock->sslData)
        return APR_EINVAL;

    if (sock->sslData->sslErr)
        return sock->sslData->sslErr;

    return APR_SUCCESS;
}

#endif
