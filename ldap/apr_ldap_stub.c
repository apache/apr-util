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
#include "apu.h"
#include "apu_config.h"
#include "apr_ldap.h"
#include "apu_internal.h"
#include "apr_atomic.h"
#include "apr_dso.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apu_version.h"

#if APR_HAS_LDAP

#if APU_DSO_BUILD

static apr_pool_t *global;
static struct apr__ldap_dso_fntable *lfn = NULL;
static apr_uint32_t in_init = 0, initialised = 0;

static apr_status_t apr_ldap_term(void *ptr)
{
    if (apr_atomic_inc32(&in_init)) {
        while (apr_atomic_read32(&in_init) > 1); /* wait until we get fully inited */
    }

    /* Reference count - cleanup when last reference is cleaned up */
    if (!apr_atomic_dec32(&initialised)) {

        apr_pool_destroy(global);

        /* set statics to NULL so init can work again */
        global = NULL;
        lfn = NULL;
    }

    apr_atomic_dec32(&in_init);

    /* Everything else we need is handled by cleanups registered
     * when we created mutexes and loaded DSOs
     */
    return APR_SUCCESS;
}

static apr_status_t load_ldap(apr_pool_t *pool, const apr_ldap_driver_t **driver, apu_err_t *err)
{
    char *modname;
    apr_dso_handle_sym_t symbol;
    apr_status_t rv;

    /* deprecate in 2.0 - permit implicit initialization */
    apu_dso_init(pool);

    rv = apu_dso_mutex_lock();
    if (rv) {
        return rv;
    }

#if defined(WIN32)
    modname = "apr_ldap-" APR_STRINGIFY(APR_MAJOR_VERSION) ".dll";
#else
    modname = "apr_ldap-" APR_STRINGIFY(APR_MAJOR_VERSION) ".so";
#endif
    rv = apu_dso_load(NULL, &symbol, modname, "apr__ldap_fns", pool, err);
    if (rv == APR_SUCCESS || APR_EINIT == rv) {
        lfn = symbol;
        rv = APR_SUCCESS;
    }

    if (driver) {
        *driver = (apr_ldap_driver_t *)lfn;
    }

    apu_dso_mutex_unlock();

    return rv;
}

#define LOAD_LEGACY_STUB(pool, err, failres) \
    if (!lfn && (apr_ldap_get_driver(pool, NULL, err) != APR_SUCCESS)) \
        return failres;

#define LOAD_LDAP_STUB(pool, err) \
    { \
        apr_status_t status; \
        if (!lfn && ((status = apr_ldap_get_driver(pool, NULL, err)) != APR_SUCCESS)) \
            return status; \
    }

#define CHECK_LDAP_STUB(failres) \
    if (!lfn) \
        return failres;

APU_DECLARE(apr_status_t) apr_ldap_get_driver(apr_pool_t *pool,
                                              const apr_ldap_driver_t **driver,
                                              apu_err_t *err)
{
    apr_status_t status = APR_EREINIT;

    if (apr_atomic_inc32(&in_init)) {
        while (apr_atomic_read32(&in_init) > 1); /* wait until we get fully inited */
    }

    /* Reference count increment... */
    if (!apr_atomic_inc32(&initialised)) {

        apr_pool_create_unmanaged(&global);

        status = load_ldap(global, driver, err);

    }

    apr_pool_cleanup_register(pool, NULL, apr_ldap_term,
                              apr_pool_cleanup_null);

    apr_atomic_dec32(&in_init);

    return status;
}

/* Legacy API */

APU_DECLARE_LDAP(int) apr_ldap_info(apr_pool_t *pool,
                                    apr_ldap_err_t **result_err)
{
    LOAD_LEGACY_STUB(pool, NULL, -1);
    return lfn->info(pool, result_err);
}

APU_DECLARE_LDAP(int) apr_ldap_init(apr_pool_t *pool,
                                    LDAP **ldap,
                                    const char *hostname,
                                    int portno,
                                    int secure,
                                    apr_ldap_err_t **result_err)
{
    LOAD_LEGACY_STUB(pool, NULL, -1);
    return lfn->init(pool, ldap, hostname, portno, secure, result_err);
}

APU_DECLARE_LDAP(int) apr_ldap_ssl_init(apr_pool_t *pool,
                                        const char *cert_auth_file,
                                        int cert_file_type,
                                        apr_ldap_err_t **result_err)
{
    LOAD_LEGACY_STUB(pool, NULL, -1);
    return lfn->ssl_init(pool, cert_auth_file, cert_file_type, result_err);
}

APU_DECLARE_LDAP(int) apr_ldap_ssl_deinit(void)
{
    if (!lfn)
        return -1;
    return lfn->ssl_deinit();
}

APU_DECLARE_LDAP(int) apr_ldap_get_option(apr_pool_t *pool,
                                          LDAP *ldap,
                                          int option,
                                          void *outvalue,
                                          apr_ldap_err_t **result_err)
{
    LOAD_LEGACY_STUB(pool, NULL, -1);
    return lfn->get_option(pool, ldap, option, outvalue, result_err);
}

APU_DECLARE_LDAP(int) apr_ldap_set_option(apr_pool_t *pool,
                                          LDAP *ldap,
                                          int option,
                                          const void *invalue,
                                          apr_ldap_err_t **result_err)
{
    LOAD_LEGACY_STUB(pool, NULL, -1);
    return lfn->set_option(pool, ldap, option, invalue, result_err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_rebind_init(apr_pool_t *pool)
{
    LOAD_LEGACY_STUB(pool, NULL, APR_EGENERAL);
    return lfn->rebind_init(pool);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_rebind_add(apr_pool_t *pool,
                                                   LDAP *ld,
                                                   const char *bindDN,
                                                   const char *bindPW)
{
    LOAD_LEGACY_STUB(pool, NULL, APR_EGENERAL);
    return lfn->rebind_add(pool, ld, bindDN, bindPW);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_rebind_remove(LDAP *ld)
{
    if (!lfn)
        return APR_EGENERAL;
    return lfn->rebind_remove(ld);
}

/* Current API */

APU_DECLARE_LDAP(apr_status_t) apr_ldap_initialise(apr_pool_t *pool,
                                                   apr_ldap_t **ldap,
                                                   apu_err_t *err)
{
    LOAD_LDAP_STUB(pool, err);
    return lfn->initialise(pool, ldap, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_option_get(apr_pool_t *pool,
                                                   apr_ldap_t *ldap,
                                                   int option,
                                                   apr_ldap_opt_t *outvalue,
                                                   apu_err_t *err)
{
    LOAD_LDAP_STUB(pool, err);
    return lfn->option_get(pool, ldap, option, outvalue, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_option_set(apr_pool_t *pool,
                                                   apr_ldap_t *ldap,
                                                   int option,
                                                   const apr_ldap_opt_t *invalue,
                                                   apu_err_t *err)
{
    LOAD_LDAP_STUB(pool, err);
    return lfn->option_set(pool, ldap, option, invalue, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_connect(apr_pool_t *pool,
                                                apr_ldap_t *ldap,
                                                apr_interval_time_t timeout,
                                                apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->connect(pool, ldap, timeout, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_prepare(apr_pool_t *pool,
                                                apr_ldap_t *ldap,
                                                apr_ldap_prepare_cb prepare_cb,
                                                void *prepare_ctx)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->prepare(pool, ldap, prepare_cb, prepare_ctx);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_process(apr_pool_t *pool,
                                                apr_ldap_t *ldap,
                                                apr_interval_time_t timeout,
                                                apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->process(pool, ldap, timeout, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_result(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               apr_interval_time_t timeout,
                                               apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->result(pool, ldap, timeout, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_poll(apr_pool_t *pool,
                                             apr_ldap_t *ldap,
                                             apr_pollcb_t *poll,
                                             apr_interval_time_t timeout,
                                             apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->poll(pool, ldap, poll, timeout, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_bind(apr_pool_t *pool,
                                             apr_ldap_t *ldap,
                                             const char *mech,
                                             apr_ldap_bind_interact_cb *interact_cb,
                                             void *interact_ctx,
                                             apr_interval_time_t timeout,
                                             apr_ldap_bind_cb bind_cb, void *bind_ctx,
                                             apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->bind(pool, ldap, mech, interact_cb, interact_ctx, timeout, bind_cb, bind_ctx, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_compare(apr_pool_t *pool,
                                                apr_ldap_t *ldap,
                                                const char *dn,
                                                const char *attr,
                                                const apr_buffer_t *bval,
                                                apr_array_header_t *serverctrls,
                                                apr_array_header_t *clientctrls,
                                                apr_interval_time_t timeout,
                                                apr_ldap_compare_cb compare_cb, void *ctx,
                                                apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->compare(pool, ldap, dn, attr, bval, serverctrls, clientctrls, timeout, compare_cb, ctx, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_search(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               const char *dn,
                                               apr_ldap_search_scope_e scope,
                                               const char *filter,
                                               const char **attrs,
                                               apr_ldap_switch_e attrsonly,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apr_interval_time_t timeout,
                                               apr_ssize_t sizelimit,
                                               apr_ldap_search_result_cb search_result_cb,
                                               apr_ldap_search_entry_cb search_entry_cb,
                                               void *search_ctx,
                                               apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->search(pool, ldap, dn, scope, filter, attrs, attrsonly, serverctrls, clientctrls, timeout, sizelimit, search_result_cb, search_entry_cb, search_ctx, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_add(apr_pool_t *pool,
                                            apr_ldap_t *ldap,
                                            const char *dn,
                                            apr_array_header_t *adds,
                                            apr_array_header_t *serverctrls,
                                            apr_array_header_t *clientctrls,
                                            apr_interval_time_t timeout,
                                            apr_ldap_add_cb add_cb, void *ctx,
                                            apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->add(pool, ldap, dn, adds, serverctrls, clientctrls, timeout, add_cb, ctx, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_modify(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               const char *dn,
                                               apr_array_header_t *mods,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apr_interval_time_t timeout,
                                               apr_ldap_modify_cb modify_cb, void *ctx,
                                               apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->modify(pool, ldap, dn, mods, serverctrls, clientctrls, timeout, modify_cb, ctx, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_rename(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               const char *dn, const char *newrdn, const char *newparent,
                                               apr_ldap_rename_e flags,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apr_interval_time_t timeout,
                                               apr_ldap_rename_cb rename_cb, void *ctx,
                                               apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->rename(pool, ldap, dn, newrdn, newparent, flags, serverctrls, clientctrls, timeout, rename_cb, ctx, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_delete(apr_pool_t *pool,
                                               apr_ldap_t *ldap,
                                               const char *dn,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apr_interval_time_t timeout,
                                               apr_ldap_delete_cb delete_cb, void *ctx,
                                               apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->delete(pool, ldap, dn, serverctrls, clientctrls, timeout, delete_cb, ctx, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_extended(apr_pool_t *pool,
                                                 apr_ldap_t *ldap,
                                                 const char *oid,
                                                 apr_buffer_t *data,
                                                 apr_array_header_t *serverctrls,
                                                 apr_array_header_t *clientctrls,
                                                 apr_interval_time_t timeout,
                                                 apr_ldap_extended_cb ext_cb, void *ctx,
                                                 apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->extended(pool, ldap, oid, data, serverctrls, clientctrls, timeout, ext_cb, ctx, err);
}

APU_DECLARE_LDAP(apr_status_t) apr_ldap_unbind(apr_ldap_t *ldap,
                                               apr_array_header_t *serverctrls,
                                               apr_array_header_t *clientctrls,
                                               apu_err_t *err)
{
    CHECK_LDAP_STUB(APR_EINIT);
    return lfn->unbind(ldap, serverctrls, clientctrls, err);
}


#endif /* APU_DSO_BUILD */

#endif /* APR_HAS_LDAP */

