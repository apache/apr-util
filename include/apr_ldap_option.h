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

/**
 * @file apr_ldap_option.h
 * @brief  APR-UTIL LDAP ldap_*_option() functions
 */
#ifndef APR_LDAP_OPTION_H
#define APR_LDAP_OPTION_H

/**
 * @defgroup APR_Util_LDAP LDAP
 * @ingroup APR_Util
 * @{
 */

#include "apr_ldap.h"

#if APR_HAS_LDAP

/*
 * The following defines handle the different TLS certificate
 * options available. If these options are missing, APR will try and
 * emulate support for this using the derecated ldap_start_tls_s()
 * function.
 */
#ifdef LDAP_OPT_X_TLS_NEVER
#define APR_LDAP_OPT_TLS_NEVER LDAP_OPT_X_TLS_NEVER
#else
#define APR_LDAP_OPT_TLS_NEVER 0
#endif

#ifdef LDAP_OPT_X_TLS_HARD
#define APR_LDAP_OPT_TLS_HARD LDAP_OPT_X_TLS_HARD
#else
#define APR_LDAP_OPT_TLS_HARD 1
#endif

#ifdef LDAP_OPT_X_TLS_DEMAND
#define APR_LDAP_OPT_TLS_DEMAND LDAP_OPT_X_TLS_DEMAND
#else
#define APR_LDAP_OPT_TLS_DEMAND 2
#endif

#ifdef LDAP_OPT_X_TLS_ALLOW
#define APR_LDAP_OPT_TLS_ALLOW LDAP_OPT_X_TLS_ALLOW
#else
#define APR_LDAP_OPT_TLS_ALLOW 3
#endif

#ifdef LDAP_OPT_X_TLS_TRY
#define APR_LDAP_OPT_TLS_TRY LDAP_OPT_X_TLS_TRY
#else
#define APR_LDAP_OPT_TLS_TRY 4
#endif


/**
 * APR LDAP get option function
 *
 * This function gets option values from a given LDAP session if
 * one was specified. It maps to the native ldap_get_option() function.
 * @param pool The pool to use
 * @param ldap The LDAP handle
 * @param option The LDAP_OPT_* option to return
 * @param outvalue The value returned (if any)
 * @param result_err The apr_ldap_err_t structure contained detailed results
 *        of the operation.
 */
APU_DECLARE(int) apr_ldap_get_option(apr_pool_t *pool,
                                     LDAP *ldap,
                                     int option,
                                     void *outvalue,
                                     apr_ldap_err_t **result_err);

/**
 * APR LDAP set option function
 * 
 * This function sets option values to a given LDAP session if
 * one was specified. It maps to the native ldap_set_option() function.
 * 
 * Where an option is not supported by an LDAP toolkit, this function
 * will try and apply legacy functions to achieve the same effect,
 * depending on the platform.
 * @param pool The pool to use
 * @param ldap The LDAP handle
 * @param option The LDAP_OPT_* option to set
 * @param invalue The value to set
 * @param result_err The apr_ldap_err_t structure contained detailed results
 *        of the operation.
 */
APU_DECLARE(int) apr_ldap_set_option(apr_pool_t *pool,
                                     LDAP *ldap,
                                     int option,
                                     const void *invalue,
                                     apr_ldap_err_t **result_err);

#endif /* APR_HAS_LDAP */

/** @} */

#endif /* APR_LDAP_OPTION_H */

