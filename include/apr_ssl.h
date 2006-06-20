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

#ifndef APR_SSL_H
#define APR_SSL_H

#include "apu.h"
#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_network_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file apr_ssl.h
 * @brief APR-UTIL SSL socket functions
 */
/** 
 * @defgroup APR_Util_SSL SSL socket routines
 * @ingroup APR_Util
 * @{
 */

/**
 * Structure for referencing an ssl "factory"
 */
typedef struct apr_ssl_factory   apr_ssl_factory_t;

/**
 * Structure for referencing an ssl socket. These are created
 * by referncing an apr_ssl_factory.
 */
typedef struct apr_ssl_socket    apr_ssl_socket_t;

APU_DECLARE(apr_status_t) apr_ssl_factory_create(apr_ssl_factory_t **,
                                                 const char *, const char *, const char *, apr_pool_t *);



APU_DECLARE(apr_status_t) apr_ssl_socket_create(apr_ssl_socket_t **,
                                                int, int, int,
                                                apr_ssl_factory_t *,
                                                apr_pool_t *);

APU_DECLARE(apr_status_t) apr_ssl_socket_close(apr_ssl_socket_t *);

APU_DECLARE(apr_status_t) apr_ssl_socket_connect(apr_ssl_socket_t *, apr_sockaddr_t *);

APU_DECLARE(apr_status_t) apr_ssl_socket_send(apr_ssl_socket_t *,
                                              const char *,
                                              apr_size_t *);

APU_DECLARE(apr_status_t) apr_ssl_socket_recv(apr_ssl_socket_t *,
                                              char *, apr_size_t *);

APU_DECLARE(apr_status_t) apr_ssl_socket_bind(apr_ssl_socket_t *, apr_sockaddr_t *);

APU_DECLARE(apr_status_t) apr_ssl_socket_listen(apr_ssl_socket_t *, apr_int32_t);

APU_DECLARE(apr_status_t) apr_ssl_socket_accept(apr_ssl_socket_t **,
                                                apr_ssl_socket_t *,
                                                apr_pool_t *);
/** @} */
#ifdef __cplusplus
}
#endif

#endif	/* !APR_SSL_H */
