/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
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

#ifndef APR_OPTIONAL_H
#define APR_OPTIONAL_H

#include "apu.h"
/** 
 * @file apr_optional.h
 * @brief APR-UTIL registration of functions exported by modules
 */
#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup APR_Util_Opt Optional Functions
 * @ingroup APR_Util
 *
 * Typesafe registration and retrieval of functions that may not be present
 * (i.e. functions exported by optional modules)
 * @{
 */

/**
 * The type of an optional function.
 * @param name The name of the function
 */
#define APR_OPTIONAL_FN_TYPE(name) apr_OFN_##name##_t

/**
 * Declare an optional function.
 * @param ret The return type of the function
 * @param name The name of the function
 * @param args The function arguments (including brackets)
 */
#define APR_DECLARE_OPTIONAL_FN(ret,name,args) \
typedef ret (APR_OPTIONAL_FN_TYPE(name)) args

/**
 * XXX: This doesn't belong here, then!
 * Private function! DO NOT USE! 
 * @internal
 */

typedef void (apr_opt_fn_t)(void);
/** @internal */
APU_DECLARE_NONSTD(void) apr_dynamic_fn_register(const char *szName, 
                                                  apr_opt_fn_t *pfn);
    
/** @internal deprecated function, see apr_dynamic_fn_register */
APU_DECLARE_NONSTD(void) apr_register_optional_fn(const char *szName, 
                                                  apr_opt_fn_t *pfn);

/**
 * Register an optional function. This can be later retrieved, type-safely, by
 * name. Like all global functions, the name must be unique. Note that,
 * confusingly but correctly, the function itself can be static!
 * @param name The name of the function
 */
#define APR_REGISTER_OPTIONAL_FN(name) \
    (((void (*)(const char *, APR_OPTIONAL_FN_TYPE(name) *)) \
               &apr_dynamic_fn_register)(#name,name))

/** @internal
 * Private function! DO NOT USE! 
 */
APU_DECLARE(apr_opt_fn_t *) apr_dynamic_fn_retrieve(const char *szName);

/** @internal deprecated function, see apr_dynamic_fn_retrieve */
APU_DECLARE(apr_opt_fn_t *) apr_retrieve_optional_fn(const char *szName);

/**
 * Retrieve an optional function. Returns NULL if the function is not present.
 * @param name The name of the function
 */
#define APR_RETRIEVE_OPTIONAL_FN(name) \
	(APR_OPTIONAL_FN_TYPE(name) *)apr_dynamic_fn_retrieve(#name)

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* APR_OPTIONAL_H */
