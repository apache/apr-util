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

#ifndef APR_SSL_WINSOCK_PRIVATE_H
#define APR_SSL_WINSOCK_PRIVATE_H

#ifdef APU_HAVE_WINSOCKSSL

//#include <openssl/ssl.h>

struct apu_ssl_data {
//    SSL_CTX *ctx;
//    const EVP_MD *md;
    int dummy;
};

struct apu_ssl_socket_data {
//    SSL     *ssl;
    int      err;    /** error code returned by function call */
    int      sslErr; /** SSL_get_error() code */ 
};


#endif /* APU_HAVE_WINSOCKSSL */

#endif /* ! APR_SSL_WINSOCK_PRIVATE_H */
