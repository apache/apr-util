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

/*
 * apr_ldap_init.c: LDAP v2/v3 common initialise
 * 
 * Original code from auth_ldap module for Apache v1.3:
 * Copyright 1998, 1999 Enbridge Pipelines Inc. 
 * Copyright 1999-2001 Dave Carrigan
 */

#include <apu.h>
#include <apr_ldap.h>
#include <apr_errno.h>
#include <apr_pools.h>
#include <apr_strings.h>

#if APR_HAS_LDAP

/**
 * APR LDAP SSL Initialise function
 *
 * This function sets up any SSL certificate parameters as
 * required by the application. It should be called once on
 * system initialisation.
 *
 * If SSL support is not available on this platform, or a problem
 * was encountered while trying to set the certificate, the function
 * will return APR_EGENERAL. Further LDAP specific error information
 * can be found in result_err.
 */
APU_DECLARE(int) apr_ldap_ssl_init(apr_pool_t *pool,
                                   const char *cert_auth_file,
                                   int cert_file_type,
                                   apr_ldap_err_t **result_err) {

    apr_ldap_err_t *result;

    if (cert_auth_file) {
        return apr_ldap_ssl_add_cert(pool,
                                     cert_auth_file,
                                     cert_file_type,
                                     result_err);
    }
    else {
        result = (apr_ldap_err_t *)apr_pcalloc(pool, sizeof(apr_ldap_err_t));
        *result_err = result;
#if APR_HAS_LDAP_SSL /* compiled with ssl support */

        /* Novell needs special initialisation */
        if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_NOVELL)) {

#if APR_HAS_LDAPSSL_CLIENT_INIT
            result->rc = ldapssl_client_init(NULL, NULL);

            if (LDAP_SUCCESS == result->rc) {
                return APR_SUCCESS;
            }
            else {
                result->msg = ldap_err2string(result-> rc);
                result->reason = apr_pstrdup (pool, "LDAP: Could not "
                                                    "initialize SSL");
                return APR_EGENERAL;
            }
#else
            result->reason = "LDAP: ldapssl_client_init() function not "
                             "supported by this Novell SDK. SSL not "
                             "initialised";
            result->rc = -1;
#endif

        }

#endif
    }

    /* if no cert_auth_file was passed, we assume SSL support
     * is possible, as we have not been specifically told otherwise.
     */
    return APR_SUCCESS;

} 


/**
 * APR LDAP SSL add client certificate function.
 *
 * This function sets up an optional client certificate to be used
 * when connecting to the remote LDAP server.
 * If SSL support is not available on this platform, or a problem
 * was encountered while trying to set the certificate, the function
 * will return APR_EGENERAL. Further LDAP specific error information
 * can be found in result_err.
 */
APU_DECLARE(int) apr_ldap_ssl_add_cert(apr_pool_t *pool,
                                       const char *cert_auth_file,
                                       int cert_file_type,
                                       apr_ldap_err_t **result_err) {

    apr_ldap_err_t *result = (apr_ldap_err_t *)apr_pcalloc(pool, sizeof(apr_ldap_err_t));
    *result_err = result;

    if (cert_auth_file) {
#if APR_HAS_LDAP_SSL /* compiled with ssl support */

        /* Netscape SDK */
        if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_NETSCAPE)) {

#if APR_HAS_LDAP_SSL_CLIENT_INIT
            /* Netscape sdk only supports a cert7.db file 
             */
            if (cert_file_type == APR_LDAP_CA_TYPE_CERT7_DB) {
                result->rc = ldapssl_client_init(cert_auth_file, NULL);
            }
            else {
                result->reason = "LDAP: Invalid certificate type: "
                                 "CERT7_DB type required";
                result->rc = -1;
            }
#else
            result->reason = "LDAP: ldapssl_client_init() function not "
                             "supported by this Netscape SDK. Certificate "
                             "authority file not set";
            result->rc = -1;
#endif

        }

        /* Novell SDK */
        else if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_NOVELL)) {

#if APR_HAS_LDAPSSL_CLIENT_INIT && APR_HAS_LDAPSSL_ADD_TRUSTED_CERT && APR_HAS_LDAPSSL_CLIENT_DEINIT
            /* Novell SDK supports DER or BASE64 files
             */
            if (cert_file_type == APR_LDAP_CA_TYPE_DER  ||
                cert_file_type == APR_LDAP_CA_TYPE_BASE64 ) {

                result->rc = ldapssl_client_init(NULL, NULL);
                if (LDAP_SUCCESS == result->rc) {
                    if (cert_file_type == APR_LDAP_CA_TYPE_BASE64) {
                        result->rc = ldapssl_add_trusted_cert((void*)cert_auth_file, 
                                                  LDAPSSL_CERT_FILETYPE_B64);
                    }
                    else {
                        result->rc = ldapssl_add_trusted_cert((void*)cert_auth_file, 
                                                  LDAPSSL_CERT_FILETYPE_DER);
                    }

                    if (LDAP_SUCCESS != result->rc) {
                        ldapssl_client_deinit();
                        result->reason = apr_psprintf(pool, 
                                                      "LDAP: Invalid certificate "
                                                      "or path: Could not add "
                                                      "trusted cert %s", 
                                                      cert_auth_file);
                    }
                }
            }
            else {
                result->reason = "LDAP: Invalid certificate type: "
                                 "DER or BASE64 type required";
                result->rc = -1;
            }

#else
            result->reason = "LDAP: ldapssl_client_init(), "
                             "ldapssl_add_trusted_cert() or "
                             "ldapssl_client_deinit() functions not supported "
                             "by this Novell SDK. Certificate authority file "
                             "not set";
            result->rc = -1;
#endif
        }

        /* openldap SDK */
        else if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_OPENLDAP)) {
#ifdef LDAP_OPT_X_TLS_CACERTFILE

            /* OpenLDAP SDK supports BASE64 files
             */
            if (cert_file_type == APR_LDAP_CA_TYPE_BASE64) {
                result->rc = ldap_set_option(NULL,
                                             LDAP_OPT_X_TLS_CACERTFILE,
                                             cert_auth_file);
            }
            else {
                result->reason = "LDAP: Invalid certificate type: "
                                 "BASE64 type required";
                result->rc = -1;
            }

#else
            result->reason = "LDAP: LDAP_OPT_X_TLS_CACERTFILE not "
                             "defined by this OpenLDAP SDK. Certificate "
                             "authority file not set";
            result->rc = -1;
#endif
        }

        /* microsoft SDK */
        else if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_MICROSOFT)) {

            /* Microsoft SDK use the registry certificate store - always
             * assume support is always available
             */
            result->rc = LDAP_SUCCESS;

        }

        /* Sun SDK */
        else if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_SUN)) {
            result->reason = "LDAP: Attempt to set certificate store failed. "
                             "APR does not yet know how to set a certificate "
                             "store on the Sun toolkit";
            result->rc = -1;
        }

        /* SDK not recognised */
        else {

            /* unknown toolkit type, assume no support available */
            result->reason = "LDAP: Attempt to set certificate store failed. "
                             "Toolkit type not recognised by APR as supporting "
                             "SSL";
            result->rc = -1;
        }

#else  /* not compiled with SSL Support */

        result->reason = "LDAP: Attempt to set certificate store failed. "
                         "Not built with SSL support";
        result->rc = -1;

#endif /* APR_HAS_LDAP_SSL */

        if (result->rc != -1) {
            result->msg = ldap_err2string(result-> rc);
        }

        if (LDAP_SUCCESS == result->rc) {
            return APR_SUCCESS;
        }
        else {
            return APR_EGENERAL;
        }
    }

    /* if no cert_auth_file was passed, we assume SSL support
     * is possible, as we have not been specifically told otherwise.
     */
    return APR_SUCCESS;

} 


/**
 * APR LDAP SSL De-Initialise function
 *
 * This function tears down any SSL certificate setup previously
 * set using apr_ldap_ssl_init(). It should be called to clean
 * up if a graceful restart of a service is attempted.
 *
 * This function only does anything on Netware.
 *
 * @todo currently we do not check whether apr_ldap_ssl_init()
 * has been called first - should we?
 */
APU_DECLARE(int) apr_ldap_ssl_deinit(void) {

#if APR_HAS_LDAP_SSL && APR_HAS_LDAPSSL_CLIENT_DEINIT
    ldapssl_client_deinit();
#endif
    return APR_SUCCESS;

}


/**
 * APR LDAP initialise function
 *
 * This function is responsible for initialising an LDAP
 * connection in a toolkit independant way. It does the
 * job of ldap_init() from the C api.
 *
 * It handles both the SSL and non-SSL case, and attempts
 * to hide the complexity setup from the user. This function
 * assumes that any certificate setup necessary has already
 * been done.
 *
 * If SSL or STARTTLS needs to be enabled, and the underlying
 * toolkit supports it, the following values are accepted for
 * secure:
 *
 * APR_LDAP_OPT_TLS_NEVER: No encryption
 * APR_LDAP_OPT_TLS_HARD: SSL encryption (ldaps://)
 * APR_LDAP_OPT_TLS_DEMAND: Force STARTTLS on ldap://
 * APR_LDAP_OPT_TLS_ALLOW: Allow STARTTLS on ldap://
 * APR_LDAP_OPT_TLS_TRY: Optionally try STARTLS on ldap://
 */
APU_DECLARE(int) apr_ldap_init(apr_pool_t *pool,
                               LDAP **ldap,
                               const char *hostname,
                               int portno,
                               int secure,
                               apr_ldap_err_t **result_err) {

    apr_ldap_err_t *result = (apr_ldap_err_t *)apr_pcalloc(pool, sizeof(apr_ldap_err_t));
    *result_err = result;

    /* clear connection requested */
    if (!secure) {
        *ldap = ldap_init((char *)hostname, portno);
    }
    else { /* ssl connnection requested */
#if APR_HAS_LDAP_SSL

        /* novell / netscape toolkit */
        if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_NOVELL) ||
            !strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_NETSCAPE)) {
#if APR_HAS_LDAPSSL_INIT
            *ldap = ldapssl_init(hostname, portno, 1);
#else
            result->reason = "LDAP: SSL not yet supported by APR on "
                             "this version of the Novell/Netscape toolkit";
            return APR_ENOTIMPL;
#endif
        }

        /* openldap toolkit */
        else if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_OPENLDAP)) {
#ifdef LDAP_OPT_X_TLS
            *ldap = ldap_init(hostname, portno);
            if (NULL != *ldap) {
                int SSLmode = secure;
                result->rc = ldap_set_option(*ldap, LDAP_OPT_X_TLS, &SSLmode);
                if (LDAP_SUCCESS != result->rc) {
                    ldap_unbind_s(*ldap);
                    result->reason = "LDAP: ldap_set_option failed, "
                                     "could not set security mode for "
                                     "apr_ldap_init()";
                    result->msg = ldap_err2string(result->rc);
                    *ldap = NULL;
                    return APR_EGENERAL;
                }
            }
#else
            result->reason = "LDAP: SSL/TLS not yet supported by APR on this "
                             "version of the OpenLDAP toolkit";
            return APR_ENOTIMPL;
#endif
        }

        /* microsoft toolkit */
        else if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_MICROSOFT)) {
            if (APR_LDAP_OPT_TLS_HARD == secure) {
#if APR_HAS_LDAP_SSLINIT
                *ldap = ldap_sslinit((char *)hostname, portno, 1);
#else
                result->reason = "LDAP: ldap_sslinit() not yet supported by APR on "
                                 "this version of the Microsoft toolkit";
                return APR_ENOTIMPL;
#endif
            }
            else {
#if APR_HAS_LDAP_START_TLS_S5
                if (APR_LDAP_OPT_TLS_DEMAND == secure) {
                    *ldap = ldap_init(hostname, portno);
                    if (NULL != *ldap) {
                        result->rc = ldap_start_tls_s(*ldap, NULL, NULL, NULL, NULL);
                        if (LDAP_SUCCESS != result->rc) {
                            ldap_unbind_s(*ldap);
                            result->reason = "LDAP: ldap_start_tls_s() failed, "
                                             "could not set STARTTLS mode for "
                                             "apr_ldap_init()";
                            result->msg = ldap_err2string(result->rc);
                            *ldap = NULL;
                            return APR_EGENERAL;
                        }
                    }
                }
                else {
                    result->reason = "LDAP: APR_LDAP_OPT_TLS_ALLOW and "
                                     "APR_LDAP_OPT_TLS_TRY are not supported "
                                     "by APR on this version of the Microsoft "
                                     "toolkit. Use APR_LDAP_OPT_TLS_DEMAND "
                                     "instead to enable STARTTLS";
                    return APR_ENOTIMPL;
                }
#else
                result->reason = "LDAP: ldap_start_tls_s() not yet supported "
                                 "by APR on this version of the Microsoft toolkit";
                return APR_ENOTIMPL;
#endif
            }
        }

        /* sun toolkit */
        else if (!strcmp(LDAP_VENDOR_NAME, APR_LDAP_VENDOR_SUN)) {
            result->reason = "LDAP: SSL not yet supported by APR on "
                             "this version of the Sun toolkit";
            return APR_ENOTIMPL;
        }

        /* unknown toolkit - return not implemented */
        else {
            return APR_ENOTIMPL;
        }
#endif /* APR_HAS_LDAP_SSL */
    }

    /* if the attempt returned a NULL object, return an error 
     * from the os as per the LDAP C SDK.
     */
    if (NULL == *ldap) {
        return apr_get_os_error();
    }
    
    /* otherwise we were successful */
    return APR_SUCCESS;

}


/**
 * APR LDAP info function
 *
 * This function returns a string describing the LDAP toolkit
 * currently in use. The string is placed inside result_err->reason.
 */
APU_DECLARE(int) apr_ldap_info(apr_pool_t *pool, apr_ldap_err_t **result_err)
{
    apr_ldap_err_t *result = (apr_ldap_err_t *)apr_pcalloc(pool, sizeof(apr_ldap_err_t));
    *result_err = result;

    result->reason = "APR LDAP: Built with "
                     LDAP_VENDOR_NAME
                     " LDAP SDK";
    return APR_SUCCESS;
    
}

#endif /* APR_HAS_LDAP */
