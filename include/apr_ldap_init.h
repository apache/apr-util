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

#ifndef APR_LDAP_INIT_H
#define APR_LDAP_INIT_H

#include "apr_ldap.h"

#if APR_HAS_LDAP

/*
 * The following defines handle the different certificate file
 * types that might be used when SSL support is included.
 */
#define APR_LDAP_CA_TYPE_UNKNOWN  0
#define APR_LDAP_CA_TYPE_DER      1
#define APR_LDAP_CA_TYPE_BASE64   2
#define APR_LDAP_CA_TYPE_CERT7_DB 3
#define APR_LDAP_CA_TYPE_SECMOD 4
#define APR_LDAP_CERT_TYPE_UNKNOWN 5
#define APR_LDAP_CERT_TYPE_DER 6
#define APR_LDAP_CERT_TYPE_BASE64 7
#define APR_LDAP_CERT_TYPE_KEY3_DB 8
#define APR_LDAP_KEY_TYPE_UNKNOWN 9
#define APR_LDAP_KEY_TYPE_DER 10
#define APR_LDAP_KEY_TYPE_BASE64 11

APU_DECLARE(int) apr_ldap_ssl_init(apr_pool_t *pool,
                                   const char *cert_auth_file,
                                   int cert_file_type,
                                   apr_ldap_err_t **result_err);

APU_DECLARE(int) apr_ldap_ssl_add_cert(apr_pool_t *pool,
                                   const char *cert_auth_file,
                                   int cert_file_type,
                                   apr_ldap_err_t **result_err);

APU_DECLARE(int) apr_ldap_ssl_deinit(void);

APU_DECLARE(int) apr_ldap_init(apr_pool_t *pool,
                               LDAP **ldap,
                               const char *hostname,
                               int portno,
                               int secure,
                               apr_ldap_err_t **result_err);

APU_DECLARE(int) apr_ldap_info(apr_pool_t *pool,
                               apr_ldap_err_t **result_err);

#endif /* APR_HAS_LDAP */

#endif /* APR_LDAP_URL_H */
