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

/**
 * @file  apu_compat.h
 * @brief APR-UTIL Compabitlity Functions
 * @deprecated These functions are only present for historical purposes
 */
#ifndef APU_COMPAT_H
#define APU_COMPAT_H
/**
 * @defgroup APR_Util_compat 1.3 Compatibility Functions
 * @ingroup APR_Util
 * @{
 */


/* Include the apr compatibility changes, since apr-util users are
 * always apr users.
 */
#include "apr_compat.h"

/* --------------------------------------------------------------------
 * redefine 1.3.x symbols to those that now live in libapr-util
 */
/** @deprecated @see apr_base64_decode */
#define ap_base64decode apr_base64_decode
/** @deprecated @see apr_base64_decode_binary */
#define ap_base64decode_binary apr_base64_decode_binary
/** @deprecated @see apr_base64_decode_len */
#define ap_base64decode_len apr_base64_decode_len
/** @deprecated @see apr_base64_encode */
#define ap_base64encode apr_base64_encode
/** @deprecated @see apr_base64_encode_binary */
#define ap_base64encode_binary apr_base64_encode_binary
/** @deprecated @see apr_base64_encode_len */
#define ap_base64encode_len apr_base64_encode_len
/** @deprecated @see apr_hook_deregister_all */
#define ap_hook_deregister_all apr_hook_deregister_all
/** @deprecated @see apr_hook_sort_register */
#define ap_hook_sort_register apr_hook_sort_register
/** @deprecated @see apr_hook_debug_show */
#define ap_show_hook apr_hook_debug_show

/* --------------------------------------------------------------------
 * the following symbols were moved from httpd-2.0/.../util_date.[ch]
 */
/** @deprecated @see apr_date_parse_http */
#define ap_parseHTTPdate apr_date_parse_http
/** @deprecated @see apr_date_checkmask */
#define ap_checkmask apr_date_checkmask

/* --------------------------------------------------------------------
 * the following symbols were moved from httpd-2.0/.../util_xml.[ch]
 */
/** @deprecated @see apr_text */
#define ap_text apr_text
/** @deprecated @see apr_text_header */
#define ap_text_header apr_text_header
/** @deprecated @see apr_text_append */
#define ap_text_append apr_text_append

/** @deprecated @see APR_XML_NS_DAV_ID */
#define AP_XML_NS_DAV_ID APR_XML_NS_DAV_ID
/** @deprecated @see APR_XML_NS_NONE */
#define AP_XML_NS_NONE APR_XML_NS_NONE
/** @deprecated @see APR_XML_NS_ERROR_BASE */
#define AP_XML_NS_ERROR_BASE APR_XML_NS_ERROR_BASE
/** @deprecated @see APR_XML_NS_IS_ERROR */
#define AP_XML_NS_IS_ERROR(e) APR_XML_NS_IS_ERROR(e)
/** @deprecated @see APR_XML_ELEM_IS_EMPTY */
#define AP_XML_ELEM_IS_EMPTY(e) APR_XML_ELEM_IS_EMPTY(e)

/** @deprecated @see apr_xml_attr */
#define ap_xml_attr apr_xml_attr
/** @deprecated @see apr_xml_elem */
#define ap_xml_elem apr_xml_elem
/** @deprecated @see apr_xml_doc */
#define ap_xml_doc apr_xml_doc

/** @deprecated @see apr_xml_to_text */
#define ap_xml_to_text apr_xml_to_text
/** @deprecated @see APR_XML_X2T_FULL */
#define AP_XML_X2T_FULL APR_XML_X2T_FULL
/** @deprecated @see APR_XML_X2T_INNER */
#define AP_XML_X2T_INNER APR_XML_X2T_INNER
/** @deprecated @see APR_XML_X2T_LANG_INNER */
#define AP_XML_X2T_LANG_INNER APR_XML_X2T_LANG_INNER
/** @deprecated @see APR_XML_X2T_FULL_NS_LANG */
#define AP_XML_X2T_FULL_NS_LANG APR_XML_X2T_FULL_NS_LANG

/** @deprecated @see apr_xml_empty_elem */
#define ap_xml_empty_elem apr_xml_empty_elem
/** @deprecated @see apr_xml_quote_string */
#define ap_xml_quote_string apr_xml_quote_string
/** @deprecated @see apr_xml_quote_elem */
#define ap_xml_quote_elem apr_xml_quote_elem
/** @deprecated @see apr_xml_insert_uri */
#define ap_xml_insert_uri apr_xml_insert_uri
/** @deprecated @see APR_XML_GET_URI_ITEM */
#define AP_XML_GET_URI_ITEM(a,i) APR_XML_GET_URI_ITEM(a,i)
/** @} */
#endif /* APU_COMPAT_H */
