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
 * Values that determine how a created factory will be used.
 */
typedef enum {
    APR_SSL_FACTORY_SERVER,   /**< Factory is for server operations */
    APR_SSL_FACTORY_CLIENT    /**< Factory is for client operations */
} apr_ssl_factory_type_e;

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
 * @brief Initialise the underlying SSL implementation in use.
 * @return APR_NOTIMPL in case of no crypto support.
 */
APU_DECLARE(apr_status_t) apr_ssl_init(void);

/**
 * @brief Attempts to create an SSL "factory". The "factory" is then 
 *        used to create sockets.
 * @param newFactory The newly created factory
 * @param privateKeyFilename Private key filename to use
 * @param certificateFilename X509 certificate file
 * @param digestTypeToUse A string identifying the type of digest scheme 
 *                        to use
 * @param purpose Constant that determines how the created factory will be used
 * @param pool The pool to use for memory allocations
 * @return an APR_ status code
 */
APU_DECLARE(apr_status_t) apr_ssl_factory_create(apr_ssl_factory_t **f,
                                                 const char *k, const char *c, 
                                                 const char *d, 
                                                 apr_ssl_factory_type_e purpose,
                                                 apr_pool_t *p);

/**
 * @brief Return the name of the library or underlying SSL
 *        implementation in use.
 * @return NULL in case of no SSL support.
 */
APU_DECLARE(const char *) apr_ssl_library_name(void);

/**
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
APU_DECLARE(apr_status_t) apr_ssl_socket_create(apr_ssl_socket_t **s,
                                                int family, int type, int proto,
                                                apr_ssl_factory_t *f,
                                                apr_pool_t *p);

/**
 * @brief Close a socket. This terminates the SSL connections as well
 *        as closing the system socket.
 * @param sock The socket to close 
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_close(apr_ssl_socket_t *sock);

/**
 * @brief Try and connect the provided SSL socket with the socket
 *        described by the provided address.
 * @param sock The SSL socket we wish to use for our side of the connection 
 * @param sa The address we wish to connect to.
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_connect(apr_ssl_socket_t *sock, 
                                                 apr_sockaddr_t *sa);

/**
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
APU_DECLARE(apr_status_t) apr_ssl_socket_send(apr_ssl_socket_t *sock,
                                              const char *buf,
                                              apr_size_t *len);

/**
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
APU_DECLARE(apr_status_t) apr_ssl_socket_recv(apr_ssl_socket_t *sock,
                                              char *buf, apr_size_t *len);

/**
 * @see apr_socket_bind
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_bind(apr_ssl_socket_t *sock, 
                                              apr_sockaddr_t *sa);

/**
 * @see apr_socket_listen
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_listen(apr_ssl_socket_t *sock, 
                                                apr_int32_t port);

/**
 * @brief Accept a new connection request on an SSL socket. This creates
 *        and returns a new SSL enabled socket. The enw socket will
 *        "belong" to the same factory that created the original socket.
 * @param newSock A copy of the socket that is connected to the socket that
 *                made the connection request.  This is the socket which should
 *                be used for all future communication.
 * @param sock The socket we are listening on.
 * @param pool The pool for the new socket.
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_accept(apr_ssl_socket_t **newSock,
                                                apr_ssl_socket_t *sock,
                                                apr_pool_t *p);

/**
 * @brief Return the error code from the underlying SSL implementation.
 * @note This is provided for completeness. Return values are specific
 *       to the underlying implentation, so this should nt be used if
 *       platform independance is desired.
 * @param sock The socket to report the error for.
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_raw_error(apr_ssl_socket_t *sock);


/**
 * @brief Add an ssl socket to a pollset.
 * @param pollset The pollset to add the socket to.
 * @param sock The ssl socket to add.
 * @note This function adds the socket with APR_POLLIN and APR_POLLOUT
 *       set.
 */
APU_DECLARE(apr_status_t) apr_pollset_add_ssl_socket(apr_pollset_t *pollset,
                                                     apr_ssl_socket_t *sock);

/**
 * @brief Remove the ssl socket from it's pollset.
 * @param sock The ssl socket to remove.
 */
APU_DECLARE(apr_status_t) apr_pollset_remove_ssl_socket(apr_ssl_socket_t *sock);

/**
 * @brief Set the required events for a socket.
 * @note These will be used when apr_pollset_poll is next called for the
 *       pollset the socket is currently attached to.
 * @param sock The socket to set events for
 * @param events The new events
 * @return APR_SUCCESS if change made. Returns EINVAL if socket is not
 *         attached to a pollset.
 */
APU_DECLARE(apr_status_t) apr_ssl_socket_set_poll_events(apr_ssl_socket_t *sock,
                                                         apr_int16_t events);


/**
 * Values that determine how a created factory will be used.
 */
typedef enum {
    APR_EVP_FACTORY_SYM,   /**< Factory is for symmetrical operations */
    APR_EVP_FACTORY_ASYM    /**< Factory is for asymmetrical operations */
} apr_evp_factory_type_e;

/**
 * Values that determine whether we need to encrypt or decrypt.
 */
typedef enum {
    APR_EVP_DECRYPT=0,
    APR_EVP_ENCRYPT=1
} apr_evp_crypt_type_e;

/**
 * Values that determine which key to use during encrypt or decrypt.
 */
typedef enum {
    APR_EVP_KEY_SYM=0,     /* Use a passphrase / symmetrical */
    APR_EVP_KEY_PUBLIC=1,  /* Use the public key / asymmetrical */
    APR_EVP_KEY_PRIVATE=2  /* Use the private key / asymetrical */
} apr_evp_crypt_key_e;

/**
 * Structure for referencing an evp "factory"
 */
typedef struct apu_evp_factory   apr_evp_factory_t;

/**
 * Structure for referencing an EVP PKEY context.
 */
typedef struct apu_evp_crypt     apr_evp_crypt_t;

/**
 * @brief Initialise the underlying crypto implementation in use.
 * @return APR_NOTIMPL in case of no crypto support.
 */
APU_DECLARE(apr_status_t) apr_evp_init(void);

/**
 * @brief Attempts to create an EVP "factory". The "factory" is then 
 *        used to create contexts to keep track of encryption.
 * @param newFactory The newly created factory
 * @param privateKeyFilename Private key filename to use for assymetrical
 *                           encryption
 * @param certificateFilename X509 certificate file to use for assymetrical
 *                            encryption
 * @param cipherName Name of cipher to use for symmetrical encryption
 * @param passphrase Passphrase to use for assymetrical encryption
 * @param purpose Constant that determines how the created factory will be used
 * @param pool The pool to use for memory allocations
 * @return an APR_ status code. APR_ENOCERT will be returned if the certificates
 *         cannot be loaded, APR_ENOCIPHER if the cipher cannot be found.
 *         APR_ENODIGEST if the digest cannot be found. APR_ENOTIMPL will
 *         be returned if not supported.
 */
APU_DECLARE(apr_status_t) apr_evp_factory_create(apr_evp_factory_t **newFactory,
                                                 const char *privateKeyFn,
                                                 const char *certFn,
                                                 const char *cipherName,
                                                 const char *passphrase,
                                                 const char *engine,
                                                 const char *digest,
                                                 apr_evp_factory_type_e purpose,
                                                 apr_pool_t *pool);

/**
 * @brief Initialise a context for encrypting arbitrary data.
 * @note If *e is NULL, a apr_evp_crypt_t will be created from a pool. If
 *       *e is not NULL, *e must point at a previously created structure.
 * @param factory The EVP factory containing keys to use.
 * @param evp The evp context returned, see note.
 * @param type Whether to encrypt or decrypt.
 * @param key Which key to use.
 * @param p The pool to use.
 * @return APR_EINIT if initialisation unsuccessful. Returns
 *         APR_ENOTIMPL if not supported.
 */
APU_DECLARE(apr_status_t) apr_evp_crypt_init(apr_evp_factory_t *f,
                                             apr_evp_crypt_t **e,
                                             apr_evp_crypt_type_e type,
                                             apr_evp_crypt_key_e key,
                                             apr_pool_t *p);

/**
 * @brief Encrypt/decrypt data provided by in, write it to out.
 * @note The number of bytes written will be written to outlen. If
 *       out is NULL, outlen will contain the maximum size of the
 *       buffer needed to hold the data, including any data
 *       generated by apr_evp_crypt_final below. If *out points
 *       to NULL, a buffer sufficiently large will be created from
 *       the pool provided. If *out points to a not-NULL value, this
 *       value will be used as a buffer instead.
 * @param evp The evp context to use.
 * @param out Address of a buffer to which data will be written,
 *        see note.
 * @param outlen Length of the output will be written here.
 * @param in Address of the buffer to read.
 * @param inlen Length of the buffer to read.
 * @return APR_EGENERAL if an error occurred. Returns APR_ENOTIMPL if
 *         not supported.
 */
APU_DECLARE(apr_status_t) apr_evp_crypt(apr_evp_crypt_t *evp,
                                        unsigned char **out,
                                        apr_size_t *outlen,
                                        const unsigned char *in,
                                        apr_size_t inlen);

/**
 * @brief Encrypt final data block, write it to out.
 * @note If necessary the final block will be written out after being
 *       padded. Typically the final block will be written to the
 *       same buffer used by apr_evp_crypt, offset by the number of
 *       bytes returned as actually written by the apr_evp_crypt()
 *       call. After this call, the context is cleaned and can be
 *       reused by apr_env_encrypt_init() or apr_env_decrypt_init().
 * @param evp The evp context to use.
 * @param out Address of a buffer to which data will be written. This
 *            buffer must already exist, and is usually the same
 *            buffer used by apr_evp_crypt(). See note.
 * @param outlen Length of the output will be written here.
 * @return APR_EGENERAL if an error occurred. Returns APR_ENOTIMPL if
 *         not supported.
 */
APU_DECLARE(apr_status_t) apr_evp_crypt_finish(apr_evp_crypt_t *evp,
                                               unsigned char *out,
                                               apr_size_t *outlen);


/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param evp The evp context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APU_DECLARE(apr_status_t) apr_evp_crypt_cleanup(apr_evp_crypt_t *e);

/**
 * @brief Clean encryption / decryption factory.
 * @note After cleanup, a factory is free to be reused if necessary.
 * @param f The factory to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APU_DECLARE(apr_status_t) apr_evp_factory_cleanup(apr_evp_factory_t *f);


/** @} */
#ifdef __cplusplus
}
#endif

#endif	/* !APR_SSL_H */
