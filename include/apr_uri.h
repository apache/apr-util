/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

/*
 * apr_uri.h: External Interface of apr_uri.c
 */

#ifndef APR_URI_H
#define APR_URI_H

#include "apu.h"

#include <apr_network_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @package Apache URI library
 */

typedef struct schemes_t schemes_t;

/** Structure to store various schemes and their default ports */
struct schemes_t {
    /** The name of the scheme */
    const char *name;
    /** The default port for the scheme */
    apr_port_t default_port;
};

#define	DEFAULT_FTP_DATA_PORT	20
#define	DEFAULT_FTP_PORT	21
#define	DEFAULT_GOPHER_PORT	70
#ifndef DEFAULT_HTTP_PORT
#define	DEFAULT_HTTP_PORT	80
#endif
#define	DEFAULT_NNTP_PORT	119
#define	DEFAULT_WAIS_PORT	210
#ifndef DEFAULT_HTTPS_PORT
#define	DEFAULT_HTTPS_PORT	443
#endif
#define	DEFAULT_SNEWS_PORT	563
#define	DEFAULT_PROSPERO_PORT	1525	/* WARNING: conflict w/Oracle */

/* Flags passed to unparse_uri_components(): */
#define UNP_OMITSITEPART	(1U<<0)	/* suppress "scheme://user@site:port" */
#define	UNP_OMITUSER		(1U<<1)	/* Just omit user */
#define	UNP_OMITPASSWORD	(1U<<2)	/* Just omit password */
#define	UNP_OMITUSERINFO	(UNP_OMITUSER|UNP_OMITPASSWORD)	/* omit "user:password@" part */
#define	UNP_REVEALPASSWORD	(1U<<3)	/* Show plain text password (default: show XXXXXXXX) */
#define UNP_OMITPATHINFO	(1U<<4)	/* Show "scheme://user@site:port" only */
#define UNP_OMITQUERY	        (1U<<5)	/* Omit the "?queryarg" from the path */

typedef struct apr_uri_components apr_uri_components;

/**
 * A structure to encompass all of the fields in a uri
 */
struct apr_uri_components {
    /** scheme ("http"/"ftp"/...) */
    char *scheme;
    /** combined [user[:password]@]host[:port] */
    char *hostinfo;
    /** user name, as in http://user:passwd@host:port/ */
    char *user;
    /** password, as in http://user:passwd@host:port/ */
    char *password;
    /** hostname from URI (or from Host: header) */
    char *hostname;
    /** port string (integer representation is in "port") */
    char *port_str;
    /** the request path (or "/" if only scheme://host was given) */
    char *path;
    /** Everything after a '?' in the path, if present */
    char *query;
    /** Trailing "#fragment" string, if present */
    char *fragment;

    /** structure returned from gethostbyname() 
     *  @defvar struct hostent *hostent */
    struct hostent *hostent;

    /** The port number, numeric, valid only if port_str != NULL */
    apr_port_t port;
    
    /** has the structure been initialized */
    unsigned is_initialized:1;

    /** has the DNS been looked up yet */
    unsigned dns_looked_up:1;
    /** has the dns been resolved yet */
    unsigned dns_resolved:1;
};

/* apr_uri.c */
/**
 * Return the default port for a given scheme.  The schemes recognized are
 * http, ftp, https, gopher, wais, nntp, snews, and prospero
 * @param scheme_str The string that contains the current scheme
 * @return The default port for this scheme
 * @deffunc apr_port_t apr_uri_default_port_for_scheme(const char *scheme_str)
 */ 
APU_DECLARE(apr_port_t) apr_uri_default_port_for_scheme(const char *scheme_str);

/**
 * Unparse a apr_uri_components structure to an URI string.  Optionally 
 * suppress the password for security reasons.
 * @param p The pool to allocate out of
 * @param uptr All of the parts of the uri
 * @param flags How to unparse the uri.  One of:
 * <PRE>
 *    UNP_OMITSITEPART        suppress "scheme://user@site:port" 
 *    UNP_OMITUSER            Just omit user 
 *    UNP_OMITPASSWORD        Just omit password 
 *    UNP_OMITUSERINFO        omit "user:password@" part 
 *    UNP_REVEALPASSWORD      Show plain text password (default: show XXXXXXXX) 
 *    UNP_OMITPATHINFO        Show "scheme://user@site:port" only 
 *    UNP_OMITQUERY           Omit the "?queryarg" from the path 
 * </PRE>
 * @return The uri as a string
 * @deffunc char * apr_uri_unparse_components(apr_pool_t *p, const apr_uri_components *uptr, unsigned flags)
 */
APU_DECLARE(char *) apr_uri_unparse_components(apr_pool_t *p, 
                                               const apr_uri_components *uptr,
                                               unsigned flags);

/**
 * Parse a given URI, fill in all supplied fields of a apr_uri_components
 * structure. This eliminates the necessity of extracting host, port,
 * path, query info repeatedly in the modules.
 * @param p The pool to allocate out of
 * @param uri The uri to parse
 * @param uptr The apr_uri_components to fill out
 * @return An HTTP status code
 * @deffunc int apr_uri_parse_components(apr_pool_t *p, const char *uri, apr_uri_components *uptr)
 */
APU_DECLARE(int) apr_uri_parse_components(apr_pool_t *p, const char *uri, 
                                          apr_uri_components *uptr);

/**
 * Special case for CONNECT parsing: it comes with the hostinfo part only
 * @param p The pool to allocate out of
 * @param hostinfo The hostinfo string to parse
 * @param uptr The apr_uri_components to fill out
 * @return An HTTP status code
 * @deffunc int apr_parse_hostinfo_components(apr_pool_t *p, const char *hostinfo, apr_uri_components *uptr)
 */
APU_DECLARE(int) apr_uri_parse_hostinfo_components(apr_pool_t *p, 
                                                   const char *hostinfo, 
                                                   apr_uri_components *uptr);

#ifdef __cplusplus
}
#endif

#endif /*APR_URI_H*/
