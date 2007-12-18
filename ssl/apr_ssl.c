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

static int sslInit = 0;

APU_DECLARE(apr_status_t) apr_ssl_init(void)
{
    if (!sslInit) {
        apr_status_t rv = apu_ssl_init();
        if (APR_SUCCESS == rv) {
            sslInit = 1;
        }
        return rv;
    }

    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apr_ssl_factory_create(apr_ssl_factory_t ** fact,
                                                 const char *privateKeyFn,
                                                 const char *certFn,
                                                 const char *digestType,
                                                 apr_ssl_factory_type_e why,
                                                 apr_pool_t * p)
{
    apr_ssl_factory_t *asf;
    apr_status_t rv;

    if (!p)
        return APR_ENOPOOL;

    asf = apr_pcalloc(p, sizeof(*asf));
    if (!asf)
        return ENOMEM;

    if (!sslInit) {
        rv = apr_ssl_init();
        if (APR_SUCCESS != rv) {
            return rv;
        }
    }

    *fact = NULL;
    asf->pool = p;
    asf->purpose = why;
    if ((rv = apu_ssl_factory_create(asf, privateKeyFn, certFn,
                                     digestType)) != APR_SUCCESS)
        return rv;

    /* should we register a cleanup here? */
    *fact = asf;
    return APR_SUCCESS;
}

APU_DECLARE(const char *) apr_ssl_library_name(void)
{
    return APU_SSL_LIBRARY;
}

#else                                /* ! APU_HAVE_SSL */

APU_DECLARE(apr_status_t) apr_ssl_init(void)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_ssl_factory_create(apr_ssl_factory_t ** fact,
                                                 const char *privateKeyFn,
                                                 const char *certFn,
                                                 const char *digestType,
                                                 apr_ssl_factory_type_e why,
                                                 apr_pool_t * p)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(const char *) apr_ssl_library_name(void)
{
    return NULL;
}

#if !APU_HAVE_OPENSSL && !APU_HAVE_WINSOCKSSL

/* default not implemented stubs when neither openssl nor winsock
 * are present.
 */

APU_DECLARE(apr_status_t) apr_evp_init(void)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_evp_factory_create(apr_evp_factory_t **newFactory,
                                                 const char *privateKeyFn,
                                                 const char *certFn,
                                                 const char *cipherName,
                                                 const char *passphrase,
                                                 const char *engine,
                                                 const char *digest,
                                                 apr_evp_factory_type_e purpose,
                                                 apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_evp_crypt_init(apr_evp_factory_t *f,
                                             apr_evp_crypt_t **e,
                                             apr_evp_crypt_type_e type,
                                             apr_evp_crypt_key_e key,
                                             apr_pool_t *p)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_evp_crypt(apr_evp_crypt_t *evp,
                                        unsigned char **out,
                                        apr_size_t *outlen,
                                        const unsigned char *in,
                                        apr_size_t inlen)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_evp_crypt_finish(apr_evp_crypt_t *evp,
                                               unsigned char *out,
                                               apr_size_t *outlen)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_evp_crypt_cleanup(apr_evp_crypt_t *e)
{
    return APR_ENOTIMPL;
}

APU_DECLARE(apr_status_t) apr_evp_factory_cleanup(apr_evp_factory_t *f)
{
    return APR_ENOTIMPL;
}

#endif                                /* !OPENSSL && !WINSOCK */

#endif                                /* APU_HAVE_SSL */
