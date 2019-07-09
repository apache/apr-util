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

#include "apr_lib.h"
#include "apu.h"
#include "apr_crypto.h"
#include "apr_crypto_internal.h"
#include "apr_strings.h"
#include "apu_config.h"

#if APU_HAVE_CRYPTO

#if APU_HAVE_OPENSSL

#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <openssl/comp.h>
#include <openssl/evp.h>

const char *apr__crypto_openssl_version(void)
{
    return OPENSSL_VERSION_TEXT;
}

apr_status_t apr__crypto_openssl_init(const char *params,
                                      const apu_err_t **result,
                                      apr_pool_t *pool)
{
    /* Both undefined (or no-op) with LibreSSL */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    CRYPTO_malloc_init();
#elif !defined(LIBRESSL_VERSION_NUMBER)
    OPENSSL_malloc_init();
#endif
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    ENGINE_load_builtin_engines();
    ENGINE_register_all_complete();

    return APR_SUCCESS;
}

apr_status_t apr__crypto_openssl_term(void)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)

#ifdef OPENSSL_FIPS
    FIPS_mode_set(0);
#endif
    CONF_modules_unload(1);
    OBJ_cleanup();
    EVP_cleanup();
#if !defined(LIBRESSL_VERSION_NUMBER)
    RAND_cleanup();
#endif
    ENGINE_cleanup();
#ifndef OPENSSL_NO_COMP
    COMP_zlib_cleanup();
#endif
#if OPENSSL_VERSION_NUMBER >= 0x1000000fL
    ERR_remove_thread_state(NULL);
#else
    ERR_remove_state(0);
#endif
    ERR_free_strings();
    CRYPTO_cleanup_all_ex_data();

#else   /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
    OPENSSL_cleanup();
#endif	/* OPENSSL_VERSION_NUMBER >= 0x10100000L */

    return APR_SUCCESS;
}

#endif /* APU_HAVE_OPENSSL */


#endif /* APU_HAVE_CRYPTO */
