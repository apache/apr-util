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
#include "apu.h"
#include "apr_ssl.h"

#ifdef APU_HAVE_SSL

#include "apr_ssl_private.h"
#include "apr_network_io.h"
#include "apr_portable.h"

#include <stdio.h>


APU_DECLARE(apr_status_t) apr_ssl_socket_create(apr_ssl_socket_t **sock,
                                                int family, int type,
                                                int protocol,
                                                apr_ssl_factory_t *asf,
                                                apr_pool_t *p)
{
    apr_ssl_socket_t *sslSock;
    apr_pool_t *thepool;

    if (!asf)
        return -1;

    thepool = p ? p : asf->pool;
    if (!thepool)
        return APR_ENOPOOL;

    sslSock = apr_pcalloc(thepool, sizeof(*sslSock));
    if (!sslSock)
        return ENOMEM;

    if (apr_socket_create(&sslSock->plain, family, type, protocol, thepool)
        != APR_SUCCESS) {
        return -1;
    }
    sslSock->pool = thepool;
    sslSock->factory = asf;
    if (apu_ssl_socket_create(sslSock, asf) != APR_SUCCESS) {
        apr_socket_close(sslSock->plain);
        return -1;
    }

    *sock = sslSock;
    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_close(apr_ssl_socket_t *sock)
{
    apr_status_t rv;
    if (!sock || !sock->sslData)
        return APR_EINVAL;

    if ((rv = apu_ssl_socket_close(sock)) != APR_SUCCESS)
        return rv;
    return apr_socket_close(sock->plain);
}

APU_DECLARE(apr_status_t) apr_ssl_socket_connect(apr_ssl_socket_t *sock,
                                                 apr_sockaddr_t *sa)
{
    apr_status_t rv;
    int sslErr;

    if (!sock || !sock->sslData || !sock->plain)
        return APR_EINVAL;

    if ((rv = apr_socket_connect(sock->plain, sa)) != APR_SUCCESS)
        return rv;
    return apu_ssl_connect(sock);
}

APU_DECLARE(apr_status_t) apr_ssl_socket_send(apr_ssl_socket_t *sock,
                                              const char *buf,
                                              apr_size_t *len)
{
    return apu_ssl_send(sock, buf, len);
}

APU_DECLARE(apr_status_t) apr_ssl_socket_recv(apr_ssl_socket_t * sock,
                                              char *buf, apr_size_t *len)
{
    return apu_ssl_recv(sock, buf, len);
}

APU_DECLARE(apr_status_t) apr_ssl_socket_bind(apr_ssl_socket_t *sock,
                                              apr_sockaddr_t *sa)
{
    return apr_socket_bind(sock->plain, sa);
}

APU_DECLARE(apr_status_t) apr_ssl_socket_listen(apr_ssl_socket_t *sock,
                                                apr_int32_t backlog)
{
    return apr_socket_listen(sock->plain, backlog);
}

APU_DECLARE(apr_status_t) apr_ssl_socket_accept(apr_ssl_socket_t **news,
                                                apr_ssl_socket_t *sock,
                                                apr_pool_t *conn)
{
    apr_status_t rv;
    apr_socket_t *newSock;
    apr_ssl_socket_t *newSSLSock;
    apr_pool_t *thepool;

    if (!sock || !sock->sslData)
        return APR_EINVAL;

    thepool = (conn ? conn : sock->pool);
    if (!thepool)
        return APR_ENOPOOL;

    rv = apr_socket_accept(&newSock, sock->plain, thepool);
    if (rv != APR_SUCCESS)
        return rv;

    newSSLSock = apr_pcalloc(thepool, sizeof(*newSSLSock));
    if (!newSSLSock) {
        apr_socket_close(newSock);
        return ENOMEM;
    }
    newSSLSock->plain = newSock;
    if (apu_ssl_accept(newSSLSock, sock, thepool) != APR_SUCCESS) {
        apr_socket_close(newSock);
        return APR_EGENERAL;
    }
    *news = newSSLSock;
    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_raw_error(apr_ssl_socket_t *sock)
{
    if (!sock)
        return APR_EINVAL;
    return apu_ssl_raw_error(sock);
}

APU_DECLARE(apr_status_t) apr_pollset_add_ssl_socket(apr_pollset_t *pollset,
                                                     apr_ssl_socket_t *sock)
{
    apr_status_t rv;
    if (sock->pollset)
        /* socket is already in a pollset - return an error... */
        return EALREADY;

    if (!sock->poll) {
        sock->poll = apr_pcalloc(sock->pool, sizeof(*sock->poll));

        sock->poll->desc_type = APR_POLL_SOCKET;
        sock->poll->desc.s = sock->plain;
        sock->poll->client_data = sock;
    }
    sock->poll->reqevents = APR_POLLIN | APR_POLLOUT;
    rv = apr_pollset_add(pollset, sock->poll);
    if (rv != APR_SUCCESS)
        sock->poll = NULL;
    sock->pollset = pollset;
    return rv;
}



APU_DECLARE(apr_status_t) apr_pollset_remove_ssl_socket(apr_ssl_socket_t *sock)
{
    apr_status_t rv;
    if (!sock->pollset)
        return EINVAL;
    rv = apr_pollset_remove(sock->pollset, sock->poll);
    sock->pollset = NULL;
    return rv;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_set_poll_events(apr_ssl_socket_t *sock,
                                                         apr_int16_t events)
{
    apr_status_t rv;
    if (!sock->poll)
        return EINVAL;
    if ((rv = apr_pollset_remove(sock->pollset, sock->poll))
         == APR_SUCCESS) {
        sock->poll->reqevents = events;
        rv = apr_pollset_add(sock->pollset, sock->poll);
    }
    return rv;
}


#else /* ! APU_HAVE_SSL */

APU_DECLARE(apr_status_t) apr_ssl_socket_create(apr_ssl_socket_t **sock,
                                                int family, int type,
                                                int protocol,
                                                apr_ssl_factory_t *asf,
                                                apr_pool_t *p)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_close(apr_ssl_socket_t *sock)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_connect(apr_ssl_socket_t *sock,
                                                 apr_sockaddr_t *sa)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_send(apr_ssl_socket_t *sock,
                                              const char *buf,
                                              apr_size_t *len)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_recv(apr_ssl_socket_t * sock,
                                              char *buf, apr_size_t *len)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_bind(apr_ssl_socket_t *sock,
                                              apr_sockaddr_t *sa)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_listen(apr_ssl_socket_t *sock,
                                                apr_int32_t backlog)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_accept(apr_ssl_socket_t **news,
                                                apr_ssl_socket_t *sock,
                                                apr_pool_t *conn)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_raw_error(apr_ssl_socket_t *sock)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_pollset_add_ssl_socket(apr_pollset_t *pollset,
                                                     apr_ssl_socket_t *sock)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_pollset_remove_ssl_socket(apr_ssl_socket_t *sock)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_socket_set_poll_events(apr_ssl_socket_t *sock,
                                                         apr_int16_t events)
{
    return APR_ENOTIMPL;
}

#endif /* APU_HAVE_SSL */
