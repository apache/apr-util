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
#include "apr_dso.h"
#include "apu.h"
#include "apu_errno.h"

#ifndef APU_INTERNAL_H
#define APU_INTERNAL_H

#if APU_DSO_BUILD

#ifdef __cplusplus
extern "C" {
#endif

/* For modular dso loading, an internal interlock to allow us to
 * continue to initialize modules by multiple threads, the caller
 * of apu_dso_load must lock first, and not unlock until any init
 * finalization is complete.
 */
apr_status_t apu_dso_init(apr_pool_t *pool);

apr_status_t apu_dso_mutex_lock(void);
apr_status_t apu_dso_mutex_unlock(void);

apr_status_t apu_dso_load(apr_dso_handle_t **dso, apr_dso_handle_sym_t *dsoptr, const char *module,
                          const char *modsym, apr_pool_t *pool, apu_err_t *err);

#if APR_HAS_LDAP

/* For LDAP internal builds, wrap our LDAP namespace */

struct apr__ldap_dso_fntable {
    /* legacy API */
    int (*info)(apr_pool_t *pool, apr_ldap_err_t **result_err);
    int (*init)(apr_pool_t *pool, LDAP **ldap, const char *hostname,
                int portno, int secure, apr_ldap_err_t **result_err);
    int (*ssl_init)(apr_pool_t *pool, const char *cert_auth_file,
                    int cert_file_type, apr_ldap_err_t **result_err);
    int (*ssl_deinit)(void);
    int (*get_option)(apr_pool_t *pool, LDAP *ldap, int option,
                      void *outvalue, apr_ldap_err_t **result_err);
    int (*set_option)(apr_pool_t *pool, LDAP *ldap, int option,
                      const void *invalue, apr_ldap_err_t **result_err);
    apr_status_t (*rebind_init)(apr_pool_t *pool);
    apr_status_t (*rebind_add)(apr_pool_t *pool, LDAP *ld,
                               const char *bindDN, const char *bindPW);
    apr_status_t (*rebind_remove)(LDAP *ld);
    /* current API */
    apr_status_t (*initialise)(apr_pool_t *pool, apr_ldap_t **ldap,
                               apu_err_t *err);
    apr_status_t (*option_get)(apr_pool_t *pool, apr_ldap_t *ldap, int option,
                               apr_ldap_opt_t *outvalue, apu_err_t *err);
    apr_status_t (*option_set)(apr_pool_t *pool, apr_ldap_t *ldap, int option,
                               const apr_ldap_opt_t *invalue, apu_err_t *err);
    apr_status_t (*connect)(apr_pool_t *pool, apr_ldap_t *ldap,
                            apr_interval_time_t timeout, apu_err_t *err);
    apr_status_t (*prepare)(apr_pool_t *pool, apr_ldap_t *ldap,
                            apr_ldap_prepare_cb prepare_cb,
                            void *prepare_ctx);
    apr_status_t (*process)(apr_pool_t *pool, apr_ldap_t *ldap,
                            apr_interval_time_t timeout, apu_err_t *err);
    apr_status_t (*result)(apr_pool_t *pool, apr_ldap_t *ldap,
                           apr_interval_time_t timeout, apu_err_t *err);
    apr_status_t (*poll)(apr_pool_t *pool, apr_ldap_t *ldap, apr_pollcb_t *poll,
                         apr_interval_time_t timeout, apu_err_t *err);
    apr_status_t (*bind)(apr_pool_t *pool, apr_ldap_t *ldap,
                         const char *mech, apr_ldap_bind_interact_cb *interact_cb,
                         void *interact_ctx, apr_interval_time_t timeout,
                         apr_ldap_bind_cb bind_cb, void *bind_ctx,
                         apu_err_t *err);
    apr_status_t (*compare)(apr_pool_t *pool, apr_ldap_t *ldap,
                            const char *dn, const char *attr,
                            const apr_buffer_t *bval,
                            apr_array_header_t *serverctrls,
                            apr_array_header_t *clientctrls,
                            apr_interval_time_t timeout,
                            apr_ldap_compare_cb compare_cb, void *ctx, apu_err_t *err);
    apr_status_t (*search)(apr_pool_t *pool, apr_ldap_t *ldap, const char *dn,
                           apr_ldap_search_scope_e scope, const char *filter,
                           const char **attrs, apr_ldap_switch_e attrsonly,
                           apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls,
                           apr_interval_time_t timeout, apr_ssize_t sizelimit,
                           apr_ldap_search_result_cb search_result_cb,          
                           apr_ldap_search_entry_cb search_entry_cb,                                          
                           void *search_ctx, apu_err_t *err);
    apr_status_t (*add)(apr_pool_t *pool, apr_ldap_t *ldap,
                        const char *dn, apr_array_header_t *adds,
                        apr_array_header_t *serverctrls,
                        apr_array_header_t *clientctrls,
                        apr_interval_time_t timeout,
                        apr_ldap_add_cb add_cb, void *ctx, apu_err_t *err);
    apr_status_t (*modify)(apr_pool_t *pool, apr_ldap_t *ldap,
                           const char *dn, apr_array_header_t *mods,
                           apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls,
                           apr_interval_time_t timeout,
                           apr_ldap_modify_cb modify_cb, void *ctx, apu_err_t *err);
    apr_status_t (*rename)(apr_pool_t *pool, apr_ldap_t *ldap,
                           const char *dn, const char *newrdn, const char *newparent,
                           apr_ldap_rename_e flags,
                           apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls,
                           apr_interval_time_t timeout,
                           apr_ldap_rename_cb rename_cb, void *ctx, apu_err_t *err);
    apr_status_t (*delete)(apr_pool_t *pool, apr_ldap_t *ldap,
                           const char *dn,
                           apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls,
                           apr_interval_time_t timeout,
                           apr_ldap_delete_cb delete_cb, void *ctx, apu_err_t *err);
    apr_status_t (*extended)(apr_pool_t *pool, apr_ldap_t *ldap,
                             const char *dn, apr_buffer_t *data,
                             apr_array_header_t *serverctrls,
                             apr_array_header_t *clientctrls,
                             apr_interval_time_t timeout,
                             apr_ldap_extended_cb ext_cb, void *ctx, apu_err_t *err);
    apr_status_t (*unbind)(apr_ldap_t *ldap, apr_array_header_t *serverctrls,
                           apr_array_header_t *clientctrls, apu_err_t *err);
};

#endif /* APR_HAS_LDAP */

#ifdef __cplusplus
}
#endif

#endif /* APU_DSO_BUILD */

#endif /* APU_INTERNAL_H */

