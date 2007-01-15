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

APU_DECLARE(apr_status_t) apr_ssl_factory_create(apr_ssl_factory_t **fact,
                                                 const char *privateKeyFn,
                                                 const char *certFn,
                                                 const char *digestType,
                                                 apr_pool_t *p)

{
    apr_ssl_factory_t *asf;
    apr_status_t rv;

    if (!p)
        return APR_ENOPOOL;

    asf = apr_pcalloc(p, sizeof(*asf));
    if (!asf)
        return ENOMEM;

    if (! sslInit) {
        if (apu_ssl_init() != APR_SUCCESS)
            return APR_EGENERAL; /* ?? error code ?? */
        sslInit = 1;
    }

    *fact = NULL;
    asf->pool = p;
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

#else /* ! APU_HAVE_SSL */

APU_DECLARE(apr_status_t) apr_ssl_factory_create(apr_ssl_factory_t **fact,
                                                 const char *privateKeyFn,
                                                 const char *certFn,
                                                 const char *digestType,
                                                 apr_pool_t *p)

{
    return APR_ENOTIMPL;
}

APU_DECLARE(const char *) apr_ssl_library_name(void)
{
    return NULL;
}

#endif /* APU_HAVE_SSL */
