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

#include "apr.h"
#include "apu.h"
#include "apr_ldap.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_strings.h"

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

    switch (option) {
        case APR_LDAP_OPT_TLS_CERT: {
            option_set_cert(pool, ldap, invalue, result);
            break;
        }
        case APR_LDAP_OPT_TLS: {
            option_set_tls(pool, ldap, invalue, result);
            break;
        }
        default: {

            /* set the option specified using the native LDAP function */
            result->rc = ldap_set_option(ldap, option, (void *)invalue);

            /* handle the error case */
            if (LDAP_SUCCESS != result->rc) {
                result->msg = ldap_err2string(result->rc);
                result->reason = "LDAP: Could not set an option";
            }

        }
    }

    /* handle the error case */
    if (LDAP_SUCCESS != result->rc) {
        return APR_EGENERAL;
    }

    return APR_SUCCESS;

}

/**
 * Handle APR_LDAP_OPT_TLS
 *
 * This function sets the type of TLS to be applied to this connection.
 * The options are:
 * APR_LDAP_NONE: no encryption
 * APR_LDAP_SSL: SSL encryption (ldaps://)
 * APR_LDAP_STARTTLS: STARTTLS encryption
 * APR_LDAP_STOPTLS: Stop existing TLS connecttion
 */
int option_set_tls(apr_pool_t *pool,
            LDAP *ldap,
            const void *invalue,
            apr_ldap_err_t *result) {

    int tls = * (const int *)invalue;

#if APR_HAS_LDAP_SSL /* compiled with ssl support */

    /* Netscape SDK */
#if APR_HAS_NETSCAPE_LDAPSDK
    if (APR_LDAP_SSL == tls) {
        result->rc = ldapssl_install_routines(ldap);
        if (result->rc == LDAP_SUCCESS) {
            result->rc = ldap_set_option(ldap, LDAP_OPT_SSL, LDAP_OPT_ON);
        }
        if (result->rc != LDAP_SUCCESS) {
            result->msg = ldap_err2string(result->rc);
            result->reason = "LDAP: Could not switch SSL on for this "
                             "connection.";
        }
    }
    else if (APR_LDAP_STARTTLS == tls) {
        result->reason = "LDAP: STARTTLS is not supported by the "
                         "Netscape/Mozilla SDK";
        result->rc = -1;
    }
    else if (APR_LDAP_STOPTLS == tls) {
        result->reason = "LDAP: STOPTLS is not supported by the "
                         "Netscape/Mozilla SDK";
        result->rc = -1;
    }
#endif

    /* Novell SDK */
#if APR_HAS_NOVELL_LDAPSDK
    /* ldapssl_install_routines(ldap)
     * Behavior is unpredictable when other LDAP functions are called
     * between the ldap_init function and the ldapssl_install_routines
     * function.
     * 
     * STARTTLS is supported by the ldap_start_tls_s() method
     */
    /*if ((APR_LDAP_SSL == tls) || (APR_LDAP_STARTTLS == tls)) {
        result->rc = ldapssl_install_routines(ldap);
        if (result->rc != LDAP_SUCCESS) {
            result->msg = ldap_err2string(result->rc);
            result->reason = "LDAP: Could not switch SSL on for this "
                             "connection.";
        }
    }*/
    if (APR_LDAP_STARTTLS == tls) {
        result->rc = ldapssl_start_tls(ldap);
        if (result->rc != LDAP_SUCCESS) {
            result->msg = ldap_err2string(result->rc);
            result->reason = "LDAP: Could not start TLS on this connection";
        }
    }
    else if (APR_LDAP_STOPTLS == tls) {
        result->rc = ldapssl_stop_tls(ldap);
        if (result->rc != LDAP_SUCCESS) {
            result->msg = ldap_err2string(result->rc);
            result->reason = "LDAP: Could not stop TLS on this connection";
        }
    }
#endif

    /* OpenLDAP SDK */
#if APR_HAS_OPENLDAP_LDAPSDK
#ifdef LDAP_OPT_X_TLS
    if (APR_LDAP_SSL == tls) {
        int SSLmode = LDAP_OPT_X_TLS_HARD;
        result->rc = ldap_set_option(ldap, LDAP_OPT_X_TLS, &SSLmode);
        if (LDAP_SUCCESS != result->rc) {
            result->reason = "LDAP: ldap_set_option failed. "
                             "Could not set LDAP_OPT_X_TLS to "
                             "LDAP_OPT_X_TLS_HARD";
            result->msg = ldap_err2string(result->rc);
        }   
    }
    else if (APR_LDAP_STARTTLS == tls) {
        result->rc = ldap_start_tls_s(ldap, NULL, NULL);
        if (LDAP_SUCCESS != result->rc) {
            result->reason = "LDAP: ldap_start_tls_s() failed";
            result->msg = ldap_err2string(result->rc);
        }
    }
    else if (APR_LDAP_STOPTLS == tls) {
        result->reason = "LDAP: STOPTLS is not supported by the "
                         "OpenLDAP SDK";
        result->rc = -1;
    }
#else
    result->reason = "LDAP: SSL/TLS not yet supported by APR on this "
                     "version of the OpenLDAP toolkit";
    result->rc = -1;
#endif
#endif

    /* Solaris SDK */
#if APR_HAS_SOLARIS_LDAPSDK
    if (APR_LDAP_NONE != tls) {
        result->reason = "LDAP: SSL/TLS is currently not supported by "
                         "APR on the Solaris SDK";
        result->rc = -1;
    }
#endif

    /* Microsoft SDK */
#if APR_HAS_MICROSOFT_LDAPSDK
    if (APR_LDAP_NONE == tls) {
        mode = 0;
        result->rc = ldap_set_option(ldap, LDAP_OPT_SSL, &mode);
        if (LDAP_SUCCESS != result->rc) {
            result->reason = "LDAP: an attempt to set LDAP_OPT_SSL off "
                             "failed.";
            result->msg = ldap_err2string(result->rc);
        }
    }
    else if (APR_LDAP_SSL == tls) {
        mode = 1;
        result->rc = ldap_set_option(ldap, LDAP_OPT_SSL, &mode);
        if (LDAP_SUCCESS != result->rc) {
            result->reason = "LDAP: an attempt to set LDAP_OPT_SSL on "
                             "failed.";
            result->msg = ldap_err2string(result->rc);
        }
    }
    else if (APR_LDAP_STARTTLS == tls) {
        result->rc = ldap_start_tls_s(ldap, NULL, NULL, NULL, NULL);
        if (LDAP_SUCCESS != result->rc) {
            result->reason = "LDAP: ldap_start_tls_s() failed";
            result->msg = ldap_err2string(result->rc);
        }
    }
    else if (APR_LDAP_STOPTLS == tls) {
        result->rc = ldap_stop_tls_s(ldap);
        if (LDAP_SUCCESS != result->rc) {
            result->reason = "LDAP: ldap_stop_tls_s() failed";
            result->msg = ldap_err2string(result->rc);
        }
    }
#endif

#if APR_HAS_OTHER_LDAPSDK
    if (APR_LDAP_NONE != tls) {
        result->reason = "LDAP: SSL/TLS is currently not supported by "
                         "APR on this LDAP SDK";
        result->rc = -1;
    }
#endif

#endif /* APR_HAS_LDAP_SSL */

}

/**
 * Handle APR_LDAP_OPT_TLS_CACERTFILE
 *
 * This function sets the CA certificate for further SSL/TLS connections.
 *
 * The file provided are in different formats depending on the toolkit used:
 *
 * Netscape: cert7.db file
 * Novell: PEM or DER
 * OpenLDAP: PEM (others supported?)
 * Microsoft: unknown
 * Solaris: unknown
 */
int option_set_cert(apr_pool_t *pool,
             LDAP *ldap,
             const void *invalue,
             apr_ldap_err_t *result) {

    apr_ldap_opt_tls_cert_t *cert = (apr_ldap_opt_tls_cert_t *)invalue;

#if APR_HAS_LDAP_SSL

#if APR_HAS_NETSCAPE_LDAPSDK
#if APR_HAS_LDAP_SSL_CLIENT_INIT
    const char *secmod = NULL;
    const char *key3db = NULL;
    const char *cert7db = NULL;

    /* set up cert7.db, key3.db and secmod parameters */
    while (cert) {
        switch (cert->type) {
            case APR_LDAP_CA_TYPE_CERT7_DB: {
                cert7db = cert->path;
                break;
            }
            case APR_LDAP_CA_TYPE_SECMOD: {
                secmod = cert->path;
                break;
            }
            case APR_LDAP_CERT_TYPE_KEY3_DB: {
                key3db = cert->path;
                break;
            }
            default: {
                result->rc = -1;
                result->reason = "LDAP: The Netscape/Mozilla LDAP SDK only "
                                 "understands the CERT7, KEY3 and SECMOD "
                                 "file types.";
            }
        }
        if (result->rc != LDAP_SUCCESS) {
            break;
        }
        cert = cert->next;
    }

    /* actually set the certificate parameters */
    if (result->rc == LDAP_SUCCESS) {
        if (secmod) {
            result->rc = ldapssl_advclientauth_init(cert7db, NULL,
                                                    key3db ? 1 : 0, key3db, NULL,
                                                    1, secmod, LDAPSSL_AUTH_CNCHECK);
            if (result->rc != LDAP_SUCCESS) {
                result->reason = "LDAP: ldapssl_advclientauth_init() failed.";
                result->msg = ldap_err2string(result->rc);
            }
        }
        else if (key3db) {
            result->rc = ldapssl_clientauth_init(cert7db, NULL,
                                                    1, key3db, NULL);
            if (result->rc != LDAP_SUCCESS) {
                result->reason = "LDAP: ldapssl_clientauth_init() failed.";
                result->msg = ldap_err2string(result->rc);
            }
        }
        else {
            result->rc = ldapssl_client_init(cert7db, NULL);
            if (result->rc != LDAP_SUCCESS) {
                result->reason = "LDAP: ldapssl_client_init() failed.";
                result->msg = ldap_err2string(result->rc);
            }
        }
    }
#else
    result->reason = "LDAP: ldapssl_client_init() function not "
                     "supported by this Netscape SDK. Certificate "
                     "authority file not set";
    result->rc = -1;
#endif
#endif

    /* Novell SDK */
#if APR_HAS_NOVELL_LDAPSDK
#if APR_HAS_LDAPSSL_CLIENT_INIT && APR_HAS_LDAPSSL_ADD_TRUSTED_CERT && APR_HAS_LDAPSSL_CLIENT_DEINIT
    /* The Novell library cannot support per connection certificates. Error
     * out if the ldap handle is provided.
     */
    if (ldap) {
        result->rc = -1;
        result->reason = "LDAP: The Novell LDAP SDK cannot support the setting "
                         "of certificates or keys on a per connection basis.";
    }
    /* Novell's library needs to be inititalised first */
    else {
        result->rc = ldapssl_client_init(NULL, NULL);
        if (LDAP_SUCCESS != result->rc) {
            result->msg = ldap_err2string(result-> rc);
            result->reason = apr_pstrdup (pool, "LDAP: Could not "
                                                "initialize SSL");
        }
    }
    /* set one or more certificates */
    while (LDAP_SUCCESS == result->rc && cert) {
        /* Novell SDK supports DER or BASE64 files. */
        switch (cert->type) {
            case APR_LDAP_CA_TYPE_DER: {
                result->rc = ldapssl_add_trusted_cert((void *)cert->path,
                                      LDAPSSL_CERT_FILETYPE_DER);
                result->msg = ldap_err2string(result->rc);
                break;
            }
            case APR_LDAP_CA_TYPE_BASE64: {
                result->rc = ldapssl_add_trusted_cert((void *)cert->path,
                                      LDAPSSL_CERT_FILETYPE_B64);
                result->msg = ldap_err2string(result->rc);
                break;
            }
            case APR_LDAP_CERT_TYPE_DER: {
                result->rc = ldapssl_set_client_cert((void *)cert->path,
                                      LDAPSSL_CERT_FILETYPE_DER,
                                      (void*)cert->password);
                result->msg = ldap_err2string(result->rc);
                break;
            }
            case APR_LDAP_CERT_TYPE_BASE64: {
                result->rc = ldapssl_set_client_cert((void *)cert->path,
                                      LDAPSSL_CERT_FILETYPE_B64,
                                      (void*)cert->password);
                result->msg = ldap_err2string(result->rc);
                break;
            }
            case APR_LDAP_KEY_TYPE_DER: {
                result->rc = ldapssl_set_client_private_key((void *)cert->path,
                                      LDAPSSL_CERT_FILETYPE_DER,
                                      (void*)cert->password);
                result->msg = ldap_err2string(result->rc);
                break;
            }
            case APR_LDAP_KEY_TYPE_BASE64: {
                result->rc = ldapssl_set_client_private_key((void *)cert->path,
                                      LDAPSSL_CERT_FILETYPE_B64,
                                      (void*)cert->password);
                result->msg = ldap_err2string(result->rc);
                break;
            }
            default: {
                result->rc = -1;
                result->reason = "LDAP: The Novell LDAP SDK only understands the "
                                 "DER and PEM (BASE64) file types.";
            }
        }
        if (LDAP_SUCCESS != result->rc) {
            break;
        }
        cert = cert->next;
    }
#else
    result->reason = "LDAP: ldapssl_client_init(), "
                     "ldapssl_add_trusted_cert() or "
                     "ldapssl_client_deinit() functions not supported "
                     "by this Novell SDK. Certificate authority file "
                     "not set";
    result->rc = -1;
#endif
#endif

    /* openldap SDK */
#if APR_HAS_OPENLDAP_LDAPSDK
#ifdef LDAP_OPT_X_TLS_CACERTFILE
    /* set one or more certificates */
    /* FIXME: make it support setting directories as well as files */
    while (cert) {
        /* OpenLDAP SDK supports BASE64 files. */
        switch (cert->type) {
            case APR_LDAP_CA_TYPE_BASE64: {
                result->rc = ldap_set_option(ldap, LDAP_OPT_X_TLS_CACERTFILE,
                                             (void *)cert->path);
                result->msg = ldap_err2string(result->rc);
                break;
            }
            case APR_LDAP_CERT_TYPE_BASE64: {
                result->rc = ldap_set_option(ldap, LDAP_OPT_X_TLS_CERTFILE,
                                             (void *)cert->path);
                result->msg = ldap_err2string(result->rc);
                break;
            }
            case APR_LDAP_KEY_TYPE_BASE64: {
                result->rc = ldap_set_option(ldap, LDAP_OPT_X_TLS_KEYFILE,
                                             (void *)cert->path);
                result->msg = ldap_err2string(result->rc);
                break;
            }
            default: {
                result->rc = -1;
                result->reason = "LDAP: The OpenLDAP SDK only understands the "
                                 "PEM (BASE64) file type.";
            }
        }
        if (LDAP_SUCCESS != result->rc) {
            break;
        }
        cert = cert->next;
    }
#else
    result->reason = "LDAP: LDAP_OPT_X_TLS_CACERTFILE not "
                     "defined by this OpenLDAP SDK. Certificate "
                     "authority file not set";
    result->rc = -1;
#endif
#endif

    /* microsoft SDK */
#if APR_HAS_MICROSOFT_LDAPSDK
    /* Microsoft SDK use the registry certificate store - error out here
     * with a message explaining this.
     */
    result->reason = "LDAP: CA certificates cannot be set using this method, "
                     "as they are stored in the registry instead."
    result->rc = -1;
#endif

    /* Sun SDK */
#if APR_HAS_SOLARIS_LDAPSDK
    result->reason = "LDAP: Attempt to set certificate store failed. "
                     "APR does not yet know how to set a certificate "
                     "store on the Sun toolkit";
    result->rc = -1;
#endif

    /* SDK not recognised */
#if APR_HAS_OTHER_LDAPSDK
        result->reason = "LDAP: LDAP_OPT_X_TLS_CACERTFILE not "
                         "defined by this LDAP SDK. Certificate " 
                         "authority file not set";
        result->rc = -1;
#endif

#else  /* not compiled with SSL Support */
    result->reason = "LDAP: Attempt to set certificate(s) failed. "
                     "Not built with SSL support";
    result->rc = -1;
#endif /* APR_HAS_LDAP_SSL */

    return result->rc;

}


#endif /* APR_HAS_LDAP */
