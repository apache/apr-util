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

#ifndef APR_SSL_PRIVATE_H
#define APR_SSL_PRIVATE_H

#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_ssl.h"

#include "apu.h"
#include "apr_network_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @internal */

typedef struct apu_ssl_data        apu_ssl_data_t;
typedef struct apu_ssl_socket_data apu_ssl_socket_data_t;

/**
 * SSL factory structure
 */
struct apr_ssl_factory {
    apr_pool_t     *pool;
    apu_ssl_data_t *sslData;
};

struct apr_ssl_socket {
    apr_pool_t        *pool;
    apr_socket_t      *plain;
    apr_ssl_factory_t *factory;
    apr_pollset_t     *pollset;
    apr_pollfd_t      *poll;
    int                connected;

    apu_ssl_socket_data_t *sslData;
};

/**
 * The following functions are provided by the implementations of
 * SSL libraries. They are internal ONLY and should not be referenced
 * outside of the apr_ssl code.
 */

apr_status_t apu_ssl_init(void);
apr_status_t apu_ssl_factory_create(apr_ssl_factory_t *, const char *, const char *, const char *);
apr_status_t apu_ssl_socket_create(apr_ssl_socket_t *sslSock, apr_ssl_factory_t *asf);
apr_status_t apu_ssl_socket_close(apr_ssl_socket_t *);
apr_status_t apu_ssl_connect(apr_ssl_socket_t *);
apr_status_t apu_ssl_send(apr_ssl_socket_t *, const char *, apr_size_t *);
apr_status_t apu_ssl_socket_recv(apr_ssl_socket_t *, char *, apr_size_t *);
apr_status_t apu_ssl_accept(apr_ssl_socket_t *, apr_ssl_socket_t *, apr_pool_t *);
apr_status_t apu_ssl_raw_error(apr_ssl_socket_t *);

/**
 * Descriptive name for the library we are using for SSL
 */
#ifdef APU_HAVE_OPENSSL
#define APU_SSL_LIBRARY   "openssl"
#endif

#ifdef APU_HAVE_WINSOCKSSL
#define APU_SSL_LIBRARY   "winsockssl"
#endif

#ifdef __cplusplus
}
#endif

#endif /* APR_SSL_PRIVATE_H */
