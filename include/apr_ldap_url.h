/* Copyright 2000-2004 The Apache Software Foundation
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

#ifndef APR_LDAP_URL_H
#define APR_LDAP_URL_H

#if APR_HAS_LDAP

#include "apu.h"
#include "apr_pools.h"

/*
 * types for ldap URL handling
 */
typedef struct apr_ldap_url_desc_t {
    struct  apr_ldap_url_desc_t  *lud_next;
    char    *lud_scheme;
    char    *lud_host;
    int     lud_port;
    char    *lud_dn;
    char    **lud_attrs;
    int     lud_scope;
    char    *lud_filter;
    char    **lud_exts;
    int     lud_crit_exts;
} apr_ldap_url_desc_t;

#ifndef APR_LDAP_URL_SUCCESS
#define APR_LDAP_URL_SUCCESS          0x00    /* Success */
#define APR_LDAP_URL_ERR_MEM          0x01    /* can't allocate memory space */
#define APR_LDAP_URL_ERR_PARAM        0x02    /* parameter is bad */
#define APR_LDAP_URL_ERR_BADSCHEME    0x03    /* URL doesn't begin with "ldap[si]://" */
#define APR_LDAP_URL_ERR_BADENCLOSURE 0x04    /* URL is missing trailing ">" */
#define APR_LDAP_URL_ERR_BADURL       0x05    /* URL is bad */
#define APR_LDAP_URL_ERR_BADHOST      0x06    /* host port is bad */
#define APR_LDAP_URL_ERR_BADATTRS     0x07    /* bad (or missing) attributes */
#define APR_LDAP_URL_ERR_BADSCOPE     0x08    /* scope string is invalid (or missing) */
#define APR_LDAP_URL_ERR_BADFILTER    0x09    /* bad or missing filter */
#define APR_LDAP_URL_ERR_BADEXTS      0x0a    /* bad or missing extensions */
#endif

/*
 * in url.c
 *
 */
APU_DECLARE(int) apr_ldap_is_ldap_url(const char *url);

APU_DECLARE(int) apr_ldap_is_ldaps_url(const char *url);

APU_DECLARE(int) apr_ldap_is_ldapi_url(const char *url);

APU_DECLARE(int) apr_ldap_url_parse_ext(apr_pool_t *pool,
                                        const char *url_in,
                                        apr_ldap_url_desc_t **ludpp,
                                        apr_ldap_err_t **result_err);

APU_DECLARE(int) apr_ldap_url_parse(apr_pool_t *pool,
                                    const char *url_in,
                                    apr_ldap_url_desc_t **ludpp,
                                    apr_ldap_err_t **result_err);

#endif /* APR_HAS_LDAP */

#endif /* APR_LDAP_URL_H */
