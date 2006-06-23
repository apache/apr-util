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
#include "apr_poll.h"

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

/**
 * @fn apr_status_t apr_ssl_factory_create(apr_ssl_factory_t **newFactory,
                                           const char *privateKeyFilename, 
                                           const char *certificateFilename, 
                                           const char *digestTypeToUse, 
                                           apr_pool_t *pool)
 * @brief Attempts to create an SSL "factory". The "factory" is then 
 *        used to create sockets. If a private key filename
 *        is passed then the created factory will assume it is to be used
 *        in a server context.
 * @param newFactory The newly created factory
 * @param privateKeyFilename
 * @param certificateFilename X509 certificate file
 * @param digestTypeToUse A string identifying the type of digest scheme 
 *                        to use
 * @param pool The pool to use for memory allocations
 * @return an APR_ status code
 */
APU_DECLARE(apr_status_t) apr_ssl_factory_create(apr_ssl_factory_t **,
                                                 const char *, 
                                                 const char *, 
                                                 const char *, 
                                                 apr_pool_t *);

/**
 * @fn const char * apr_ssl_library_name(void)
 * @brief Return the name of the library or underlying SSL
 *        implementation in use.
 * @return NULL in case of no SSL support.
 */
APU_DECLARE(const char *) apr_ssl_library_name(void);

/**
 * @fn apr_status_t apr_ssl_socket_create(apr_ssl_socket_t **newSock,
                                                int family,
                                                int type,
                                                int protocol,
                                                apr_ssl_factory_t *factory,
                                                apr_pool_t *pool)
 * @brief Create an ssl socket.
 * @param newSock The new socket that has been set up.
 * @param family The address family of the socket (e.g., APR_INET).
 * @param type The type of the socket (e.g., SOCK_STREAM).
 * @param protocol The protocol of the socket (e.g., APR_PROTO_TCP).
 * @param factory The factory that it will be produced from
 * @param pool The pool for the apr_socket_t and associated storage.
 * @return APR_SUCCESS on succesful creation.
 * @see apr_socket_create
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_create(apr_ssl_socket_t **,
                                                int, int, int,
                                                apr_ssl_factory_t *,
                                                apr_pool_t *);
/**
 * @fn apr_status_t apr_ssl_socket_close(apr_ssl_socket_t *sock)
 * @brief Close a socket. This terminates the SSL connections as well
 *        as closing the system socket.
 * @param sock The socket to close 
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_close(apr_ssl_socket_t *);

/**
 * @fn apr_status_t apr_ssl_socket_connect(apr_ssl_socket_t *sock, 
                                           apr_sockaddr_t *sa)
 * @brief Try and connect the provided SSL socket with the socket
 *        described by the provided address.
 * @param sock The SSL socket we wish to use for our side of the connection 
 * @param sa The address we wish to connect to.
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_connect(apr_ssl_socket_t *, 
                                                 apr_sockaddr_t *);

/**
 * @fn apr_status_t apr_ssl_socket_send(apr_ssl_socket_t *sock,
                                              const char *buf,
                                              apr_size_t *len)
 * @brief Try and send data over the SSL connection.
 * @param sock The socket to send the data over.
 * @param buf The buffer which contains the data to be sent. 
 * @param len On entry, the number of bytes to send; on exit, the number
 *            of bytes sent.
 * @remark
 * <PRE>
 * This function attempts to cope with teh various encentricities of
 * the SSL implementations in a seamless manner.
 * This functions acts like a blocking write by default.  To change 
 * this behavior, use apr_socket_timeout_set() or the APR_SO_NONBLOCK
 * socket option.
 *
 * It is possible for both bytes to be sent and an error to be returned.
 *
 * APR_EINTR is never returned.
 * </PRE>
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_send(apr_ssl_socket_t *,
                                              const char *,
                                              apr_size_t *);

/**
 * @fn apr_status_t apr_ssl_socket_recv(apr_ssl_socket_t *sock,
                                        char *buf, 
                                        apr_size_t *len);

 * @brief Read data from the SSL connection.
 * @param sock The socket to read the data from.
 * @param buf The buffer to store the data in. 
 * @param len On entry, the number of bytes to receive; on exit, the number
 *            of bytes received.
 * @remark
 * <PRE>
 * This functions acts like a blocking read by default.  To change 
 * this behavior, use apr_socket_timeout_set() or the APR_SO_NONBLOCK
 * socket option.
 * The number of bytes actually received is stored in argument 3.
 *
 * It is possible for both bytes to be received and an APR_EOF or
 * other error to be returned.
 *
 * APR_EINTR is never returned.
 * </PRE>
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_recv(apr_ssl_socket_t *,
                                              char *, apr_size_t *);

/**
 * @see apr_socket_bind
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_bind(apr_ssl_socket_t *, 
                                              apr_sockaddr_t *);

/**
 * @see apr_socket_listen
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_listen(apr_ssl_socket_t *, 
                                                apr_int32_t);

/**
 * @fn apr_status_t apr_ssl_socket_accept(apr_ssl_socket_t **newSock,
                                          apr_ssl_socket_t *sock,
                                          apr_pool_t *pool)
 * @brief Accept a new connection request on an SSL socket. This creates
 *        and returns a new SSL enabled socket. The enw socket will
 *        "belong" to the same factory that created the original socket.
 * @param newSock A copy of the socket that is connected to the socket that
 *                made the connection request.  This is the socket which should
 *                be used for all future communication.
 * @param sock The socket we are listening on.
 * @param pool The pool for the new socket.
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_accept(apr_ssl_socket_t **,
                                                apr_ssl_socket_t *,
                                                apr_pool_t *);

/**
 * @fn apr_status_t apr_ssl_socket_raw_error(apr_ssl_socket_t *sock)
 * @brief Return the error code from the underlying SSL implementation.
 * @note This is provided for completeness. Return values are specific
 *       to the underlying implentation, so this should nt be used if
 *       platform independance is desired.
 * @param sock The socket to report the error for.
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_raw_error(apr_ssl_socket_t *);


/**
 * @fn apr_status_t apr_pollset_add_ssl_socket(apr_pollset_t *pollset
                                               apr_ssl_socket_t *sock)
 * @brief Add an ssl socket to a pollset.
 * @param pollset The pollset to add the socket to.
 * @param sock The ssl socket to add.
 * @note This fucntion adds the socket with APR_POLLIN and APR_POLLOUT
 *       set.
 */
APU_DECLARE(apr_status_t) apr_pollset_add_ssl_socket(apr_pollset_t *,
                                                     apr_ssl_socket_t *);

/**
 * @fn apr_status_t apr_pollset_remove_ssl_socket(apr_ssl_socket_t *sock)
 * @brief Remove the ssl socket from it's pollset.
 * @param sock The ssl socket to remove.
 */
APU_DECLARE(apr_status_t) apr_pollset_remove_ssl_socket(apr_ssl_socket_t *);

/**
 * @fn apr_status_t apr_ssl_socket_set_poll_events(apr_ssl_socket_t *sock,
                                                   apr_int16_t events)
 * @brief Set the required events for a socket.
 * @note These will be used when apr_pollset_poll is next called for the
 *       pollset the socket is currently attached to.
 * @param sock The socket to set events for
 * @param events The new events
 * @return APR_SUCCESS if change made. Returns EINVAL if socket is not
 *         attached to a pollset.
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_set_poll_events(apr_ssl_socket_t *,
                                                         apr_int16_t);

/** @} */
#ifdef __cplusplus
}
#endif

#endif	/* !APR_SSL_H */
