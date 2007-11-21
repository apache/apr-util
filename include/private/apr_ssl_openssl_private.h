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

#ifndef APR_SSL_OPENSSL_PRIVATE_H
#define APR_SSL_OPENSSL_PRIVATE_H

#ifdef APU_HAVE_OPENSSL

#include <openssl/ssl.h>

struct apu_ssl_data {
    SSL_CTX *ctx;
    const EVP_MD *md;
};

struct apu_ssl_socket_data {
    SSL     *ssl;
    int      err;    /** error code returned by function call */
    int      sslErr; /** SSL_get_error() code */ 
};

typedef struct apu_evp_data        apu_evp_data_t;

/**
 * EVP factory structure
 */
struct apu_evp_factory {
    apr_pool_t     *pool;           /**< pool to use for memory allocations */
    apr_evp_factory_type_e purpose; /**< Purpose of the factory */
    apu_evp_data_t *evpData;        /**< Pointer to implementation specific data */
};

/**
 * Define the cipher context structure used as a handle by
 * the generic apu_evp_* functions.
 */
struct apu_evp_data {
    const EVP_CIPHER *cipher;
    const EVP_MD *md;
    unsigned char salt[8];
    unsigned char key[EVP_MAX_KEY_LENGTH];
    unsigned char iv[EVP_MAX_IV_LENGTH];
    const char *privateKeyFilename;
    const char *certificateFilename;
#if HAVE_DECL_EVP_PKEY_CTX_NEW
    SSL_CTX *sslCtx;
    SSL *ssl;
    EVP_PKEY *pubkey;
    EVP_PKEY *privkey;
#endif
};

struct apu_evp_crypt {
    apr_pool_t *pool;
    EVP_CIPHER_CTX *cipherCtx;
#if HAVE_DECL_EVP_PKEY_CTX_NEW
    EVP_PKEY_CTX *pkeyCtx;
#endif
    apr_evp_factory_type_e purpose;
    apr_evp_crypt_type_e type;
    apr_evp_crypt_key_e key;
};

#endif /* APU_HAVE_OPENSSL */

#endif /* ! APR_SSL_OPENSSL_PRIVATE_H */
