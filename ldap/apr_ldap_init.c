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
 * will return APR_EGENERAL.
 */
APU_DECLARE(int) apr_ldap_ssl_init(const char *cert_auth_file,
                                   int cert_file_type,
                                   const char **reason) {
    int rc;

    if (cert_auth_file) {
#if APR_HAS_LDAP_SSL /* compiled with ssl support */

#if APR_HAS_NETSCAPE_LDAPSDK 

        /* Netscape sdk only supports a cert7.db file 
         */
        if (st->cert_file_type == LDAP_CA_TYPE_CERT7_DB) {
            rc = ldapssl_client_init(st->cert_auth_file, NULL);
        }
        else {
            *reason = "LDAP: Invalid certificate type: "
                      "CERT7_DB type required";
            rc = -1;
        }

#elif APR_HAS_NOVELL_LDAPSDK
        
        /* Novell SDK supports DER or BASE64 files
         */
        if (cert_file_type == APR_LDAP_CA_TYPE_DER  ||
            cert_file_type == APR_LDAP_CA_TYPE_BASE64 ) {

            rc = ldapssl_client_init(NULL, NULL);
            if (LDAP_SUCCESS == rc) {
                if (st->cert_file_type == LDAP_CA_TYPE_BASE64) {
                    rc = ldapssl_add_trusted_cert(st->cert_auth_file, 
                                                  LDAPSSL_CERT_FILETYPE_B64);
                }
                else {
                    rc = ldapssl_add_trusted_cert(st->cert_auth_file, 
                                                  LDAPSSL_CERT_FILETYPE_DER);
                }

                if (LDAP_SUCCESS != rc) {
                    ldapssl_client_deinit();
                }
            }
        }
        else {
            *reason = "LDAP: Invalid certificate type: "
                             "DER or BASE64 type required");
            rc = -1;
        }

#elif APR_HAS_OPENLDAP_LDAPSDK

        /* OpenLDAP SDK supports BASE64 files
         */
        if (cert_file_type == APR_LDAP_CA_TYPE_BASE64) {
            rc = ldap_set_option(NULL, LDAP_OPT_X_TLS_CACERTFILE, cert_auth_file);
        }
        else {
            *reason = "LDAP: Invalid certificate type: "
                             "BASE64 type required";
            rc = -1;
        }

#elif APR_HAS_MICROSOFT_LDAPSDK
            
        /* Microsoft SDK use the registry certificate store - always
         * assume support is always available
         */
        rc = LDAP_SUCCESS;

#else

        /* unknown toolkit type, assume no support available */
        *reason = "LDAP: Attempt to set certificate store failed. "
                  "Toolkit type not recognised as supporting SSL.";
        rc = -1;

#endif /* APR_HAS_NETSCAPE_LDAPSDK */

#else  /* not compiled with SSL Support */

        *reason = "LDAP: Attempt to set certificate store failed. "
                  "Not built with SSL support.";
        rc = -1;

#endif /* APR_HAS_LDAP_SSL */

        if (LDAP_SUCCESS == rc) {
            return APR_SUCCESS;
        }
        else {
            /* @todo Return the LDAP error code here if not -1 */
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
APU_DECLARE(int) apr_ldap_ssl_deinit() {

#if APR_HAS_LDAP_SSL && APR_HAS_NOVELL_LDAPSDK
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
 */
APU_DECLARE(int) apr_ldap_init(LDAP **ldap,
                               const char *hostname,
                               int portno,
                               int secure,
                               const char *reason) {

    /* clear connection requested */
    if (!secure) {
        *ldap = ldap_init(hostname, portno);
    }
    else { /* ssl connnection requested */
#if APR_HAS_LDAP_SSL
#if APR_HAS_NOVELL_LDAPSDK 
        *ldap = ldapssl_init(hostname, portno, 1);
#elif APR_HAS_NETSCAPE_LDAPSDK
        *ldap = ldapssl_init(hostname, portno, 1);
#elif APR_HAS_OPENLDAP_LDAPSDK
        *ldap = ldap_init(hostname, portno);
        if (NULL != *ldap) {
            int SSLmode = LDAP_OPT_X_TLS_HARD;
            int result = ldap_set_option(*ldap, LDAP_OPT_X_TLS, &SSLmode);
            if (LDAP_SUCCESS != result) {
                ldap_unbind_s(*ldap);
                reason = "LDAP: ldap_set_option - LDAP_OPT_X_TLS_HARD failed";
                *ldap = NULL;
                /* @todo make proper APR error codes for LDAP codes */
                return APR_EGENERAL;
            }
        }
#elif APR_HAS_MICROSOFT_LDAPSDK
        *ldap = ldap_sslinit(const_cast(ldc->host), ldc->port, 1);
#else
        /* unknown toolkit - return not implemented */
        return APR_ENOTIMPL;
#endif /* APR_HAS_NOVELL_LDAPSDK */
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
 * currently in use.
 */
APU_DECLARE(int) apr_ldap_info(const char **info) {

#if APR_HAS_NETSCAPE_LDAPSDK 
    *info = ("APR LDAP: Built with Netscape LDAP SDK");
#elif APR_HAS_NOVELL_LDAPSDK
    *info = ("APR LDAP: Built with Novell LDAP SDK");
#elif APR_HAS_OPENLDAP_LDAPSDK
    *info = ("APR LDAP: Built with OpenLDAP LDAP SDK");
#elif APR_HAS_MICROSOFT_LDAPSDK
    *info = ("APR LDAP: Built with Microsoft LDAP SDK");
#else
    *info = ("APR LDAP: Built with an unknown LDAP SDK");
#endif

    return APR_SUCCESS;
    
}

#endif /* APR_HAS_LDAP */
