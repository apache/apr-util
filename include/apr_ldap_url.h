/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#ifndef APR_LDAP_URL_H
#define APR_LDAP_URL_H

#include "apr_ldap.h"

#if APR_HAS_LDAP
#if APR_HAS_LDAP_URL_PARSE

#define apr_ldap_url_desc_t             LDAPURLDesc
#define apr_ldap_is_ldap_url(url)       ldap_is_ldap_url(url)
#define apr_ldap_is_ldaps_url(url)      ldap_is_ldaps_url(url)
#define apr_ldap_is_ldapi_url(url)      ldap_is_ldapi_url(url)
#define apr_ldap_url_parse(url, ludpp)  ldap_url_parse(url, ludpp)
#define apr_ldap_free_urldesc(ludp)     ldap_free_urldesc(ludp)

#else /* ! APR_HAS_LDAP_URL_PARSE */

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

#ifndef LDAP_URL_SUCCESS
#define LDAP_URL_SUCCESS          0x00    /* Success */
#define LDAP_URL_ERR_MEM          0x01    /* can't allocate memory space */
#define LDAP_URL_ERR_PARAM        0x02    /* parameter is bad */
#define LDAP_URL_ERR_BADSCHEME    0x03    /* URL doesn't begin with "ldap[si]://" */
#define LDAP_URL_ERR_BADENCLOSURE 0x04    /* URL is missing trailing ">" */
#define LDAP_URL_ERR_BADURL       0x05    /* URL is bad */
#define LDAP_URL_ERR_BADHOST      0x06    /* host port is bad */
#define LDAP_URL_ERR_BADATTRS     0x07    /* bad (or missing) attributes */
#define LDAP_URL_ERR_BADSCOPE     0x08    /* scope string is invalid (or missing) */
#define LDAP_URL_ERR_BADFILTER    0x09    /* bad or missing filter */
#define LDAP_URL_ERR_BADEXTS      0x0a    /* bad or missing extensions */
#endif

/*
 * in url.c
 *
 * need _ext varients
 */
APU_DECLARE(int) apr_ldap_is_ldap_url(const char *url);

APU_DECLARE(int) apr_ldap_is_ldaps_url(const char *url);

APU_DECLARE(int) apr_ldap_is_ldapi_url(const char *url);

APU_DECLARE(int) apr_ldap_url_parse(const char *url, 
                                    apr_ldap_url_desc_t **ludpp);

APU_DECLARE(void) apr_ldap_free_urldesc(apr_ldap_url_desc_t *ludp);

#endif /* ! APR_HAS_LDAP_URL_PARSE */

#endif /* APR_HAS_LDAP */

#endif /* APR_LDAP_URL_H */
