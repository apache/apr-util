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

/*  apr_ldap_option.c -- LDAP options
 *
 *  The LDAP SDK allows the getting and setting of options on an LDAP
 *  connection.
 *
 */

#include <apu.h>
#include <apr_ldap.h>
#include <apr_errno.h>
#include <apr_pools.h>
#include <apr_strings.h>

#if APR_HAS_LDAP

/**
 * APR LDAP get option function
 *
 * This function gets option values from a given LDAP session if
 * one was specified.
 */
APU_DECLARE(int) apr_ldap_get_option(apr_pool_t *pool,
                                     LDAP *ldap,
                                     int option,
                                     void *outvalue,
                                     apr_ldap_err_t **result_err) {

    apr_ldap_err_t *result;

    result = (apr_ldap_err_t *)apr_pcalloc(pool, sizeof(apr_ldap_err_t));
    *result_err = result;
    if (!result) {
        return APR_ENOMEM;
    }

    /* get the option specified using the native LDAP function */
    result->rc = ldap_get_option(ldap, option, outvalue);

    /* handle the error case */
    if (LDAP_SUCCESS != result->rc) {
        result->msg = ldap_err2string(result-> rc);
        result->reason = apr_pstrdup (pool, "LDAP: Could not get an option");
        return APR_EGENERAL;
    }

    return APR_SUCCESS;

} 

/**
 * APR LDAP set option function
 *
 * This function sets option values to a given LDAP session if
 * one was specified.
 *
 * Where an option is not supported by an LDAP toolkit, this function
 * will try and apply legacy functions to achieve the same effect,
 * depending on the platform.
 */
APU_DECLARE(int) apr_ldap_set_option(apr_pool_t *pool,
                                     LDAP *ldap,
                                     int option,
                                     const void *invalue,
                                     apr_ldap_err_t **result_err) {

    apr_ldap_err_t *result;

    result = (apr_ldap_err_t *)apr_pcalloc(pool, sizeof(apr_ldap_err_t));
    *result_err = result;
    if (!result) {
        return APR_ENOMEM;
    }

    /* set the option specified using the native LDAP function */
    result->rc = ldap_set_option(ldap, option, (void *)invalue);

    /* handle the error case */
    if (LDAP_SUCCESS != result->rc) {
        result->msg = ldap_err2string(result-> rc);
        result->reason = apr_pstrdup (pool, "LDAP: Could not get an option");
        return APR_EGENERAL;
    }

    return APR_SUCCESS;

}

#endif /* APR_HAS_LDAP */
