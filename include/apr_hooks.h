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

#ifndef APR_HOOKS_H
#define APR_HOOKS_H

#include "apu.h"
/* For apr_array_header_t */
#include "apr_tables.h"

/**
 * @file apr_hooks.h
 * @brief Apache hook functions
 */

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup APR_Util_Hook Hook Functions
 * @ingroup APR_Util
 * @{
 */
/** macro to return the prototype of the hook function */    
#define APR_IMPLEMENT_HOOK_GET_PROTO(ns,link,name) \
link##_DECLARE(apr_array_header_t *) ns##_hook_get_##name(void)

/** macro to declare the hook correctly */    
#define APR_DECLARE_EXTERNAL_HOOK(ns,link,ret,name,args) \
typedef ret ns##_HOOK_##name##_t args; \
link##_DECLARE(void) ns##_hook_##name(ns##_HOOK_##name##_t *pf, \
                                      const char * const *aszPre, \
                                      const char * const *aszSucc, int nOrder); \
link##_DECLARE(ret) ns##_run_##name args; \
APR_IMPLEMENT_HOOK_GET_PROTO(ns,link,name); \
typedef struct ns##_LINK_##name##_t \
    { \
    ns##_HOOK_##name##_t *pFunc; \
    const char *szName; \
    const char * const *aszPredecessors; \
    const char * const *aszSuccessors; \
    int nOrder; \
    } ns##_LINK_##name##_t;

/** macro to declare the hook structure */    
#define APR_HOOK_STRUCT(members) \
static struct { members } _hooks;

/** macro to link the hook structure */
#define APR_HOOK_LINK(name) \
    apr_array_header_t *link_##name;

/** macro to implement the hook */
#define APR_IMPLEMENT_EXTERNAL_HOOK_BASE(ns,link,name) \
link##_DECLARE(void) ns##_hook_##name(ns##_HOOK_##name##_t *pf,const char * const *aszPre, \
                                      const char * const *aszSucc,int nOrder) \
    { \
    ns##_LINK_##name##_t *pHook; \
    if(!_hooks.link_##name) \
	{ \
	_hooks.link_##name=apr_array_make(apr_hook_global_pool,1,sizeof(ns##_LINK_##name##_t)); \
	apr_hook_sort_register(#name,&_hooks.link_##name); \
	} \
    pHook=apr_array_push(_hooks.link_##name); \
    pHook->pFunc=pf; \
    pHook->aszPredecessors=aszPre; \
    pHook->aszSuccessors=aszSucc; \
    pHook->nOrder=nOrder; \
    pHook->szName=apr_hook_debug_current; \
    if(apr_hook_debug_enabled) \
	apr_hook_debug_show(#name,aszPre,aszSucc); \
    } \
    APR_IMPLEMENT_HOOK_GET_PROTO(ns,link,name) \
    { \
        return _hooks.link_##name; \
    }

/**
 * Implement a hook that has no return code, and therefore runs all of the
 * registered functions
 * @param ns The namespace prefix of the hook functions
 * @param link The linkage declaration prefix of the hook
 * @param name The name of the hook
 * @param args_decl The declaration of the arguments for the hook
 * @param args_use The names for the arguments for the hook
 * @note The link prefix FOO corresponds to FOO_DECLARE() macros, which
 * provide export linkage from the module that IMPLEMENTs the hook, and
 * import linkage from external modules that link to the hook's module.
 */
#define APR_IMPLEMENT_EXTERNAL_HOOK_VOID(ns,link,name,args_decl,args_use) \
APR_IMPLEMENT_EXTERNAL_HOOK_BASE(ns,link,name) \
link##_DECLARE(void) ns##_run_##name args_decl \
    { \
    ns##_LINK_##name##_t *pHook; \
    int n; \
\
    if(!_hooks.link_##name) \
	return; \
\
    pHook=(ns##_LINK_##name##_t *)_hooks.link_##name->elts; \
    for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	pHook[n].pFunc args_use; \
    }

/* FIXME: note that this returns ok when nothing is run. I suspect it should
   really return decline, but that breaks Apache currently - Ben
*/
/**
 * Implement a hook that runs until one of the functions returns something
 * other than OK or DECLINE
 * @param ns The namespace prefix of the hook functions
 * @param link The linkage declaration prefix of the hook
 * @param ret Type to return
 * @param name The name of the hook
 * @param args_decl The declaration of the arguments for the hook
 * @param args_use The names for the arguments for the hook
 * @param ok Success value
 * @param decline Decline value
 * @note The link prefix FOO corresponds to FOO_DECLARE() macros, which
 * provide export linkage from the module that IMPLEMENTs the hook, and
 * import linkage from external modules that link to the hook's module.
 */
#define APR_IMPLEMENT_EXTERNAL_HOOK_RUN_ALL(ns,link,ret,name,args_decl,args_use,ok,decline) \
APR_IMPLEMENT_EXTERNAL_HOOK_BASE(ns,link,name) \
link##_DECLARE(ret) ns##_run_##name args_decl \
    { \
    ns##_LINK_##name##_t *pHook; \
    int n; \
    ret rv; \
\
    if(!_hooks.link_##name) \
	return ok; \
\
    pHook=(ns##_LINK_##name##_t *)_hooks.link_##name->elts; \
    for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	{ \
	rv=pHook[n].pFunc args_use; \
\
	if(rv != ok && rv != decline) \
	    return rv; \
	} \
    return ok; \
    }


/**
 * Implement a hook that runs until the first function returns something
 * other than the value of decline
 * @param ns The namespace prefix of the hook functions
 * @param link The linkage declaration prefix of the hook
 * @param name The name of the hook
 * @param ret Type to return
 * @param args_decl The declaration of the arguments for the hook
 * @param args_use The names for the arguments for the hook
 * @param decline Decline value
 * @note The link prefix FOO corresponds to FOO_DECLARE() macros, which
 * provide export linkage from the module that IMPLEMENTs the hook, and
 * import linkage from external modules that link to the hook's module.
 */
#define APR_IMPLEMENT_EXTERNAL_HOOK_RUN_FIRST(ns,link,ret,name,args_decl,args_use,decline) \
APR_IMPLEMENT_EXTERNAL_HOOK_BASE(ns,link,name) \
link##_DECLARE(ret) ns##_run_##name args_decl \
    { \
    ns##_LINK_##name##_t *pHook; \
    int n; \
    ret rv; \
\
    if(!_hooks.link_##name) \
	return decline; \
\
    pHook=(ns##_LINK_##name##_t *)_hooks.link_##name->elts; \
    for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	{ \
	rv=pHook[n].pFunc args_use; \
\
	if(rv != decline) \
	    return rv; \
	} \
    return decline; \
    }

    /* Hook orderings */
/** run this hook first, before ANYTHING */
#define APR_HOOK_REALLY_FIRST	(-10)
/** run this hook first */
#define APR_HOOK_FIRST		0
/** run this hook somewhere */
#define APR_HOOK_MIDDLE		10
/** run this hook after every other hook which is defined*/
#define APR_HOOK_LAST		20
/** run this hook last, after EVERYTHING */
#define APR_HOOK_REALLY_LAST	30

/**
 * The global pool used to allocate any memory needed by the hooks.
 */ 
APU_DECLARE_DATA extern apr_pool_t *apr_global_hook_pool;

/** @deprecated @see apr_hook_global_pool */
APU_DECLARE_DATA extern apr_pool_t *apr_hook_global_pool;

/**
 * A global variable to determine if debugging information about the
 * hooks functions should be printed
 */ 
APU_DECLARE_DATA extern int apr_hook_debug_enabled;

/** @deprecated @see apr_hook_debug_enabled */
APU_DECLARE_DATA extern int apr_debug_module_hooks;

/**
 * The name of the module that is currently registering a function
 */ 
APU_DECLARE_DATA extern const char *apr_hook_debug_current;

/** @deprecated @see apr_hook_debug_current */
APU_DECLARE_DATA extern const char *apr_current_hooking_module;

/**
 * Register a hook function to be sorted
 * @param szHookName The name of the Hook the function is registered for
 * @param aHooks The array which stores all of the functions for this hook
 */
APU_DECLARE(void) apr_hook_sort_register(const char *szHookName, 
                                        apr_array_header_t **aHooks);
/**
 * Sort all of the registerd functions for a given hook
 */
APU_DECLARE(void) apr_hook_sort_all(void);

/** @deprecated @see apr_hook_sort_all */
APU_DECLARE(void) apr_sort_hooks(void);

/**
 * Print all of the information about the current hook.  This is used for
 * debugging purposes.
 * @param szName The name of the hook
 * @param aszPre All of the functions in the predecessor array
 * @param aszSucc All of the functions in the successor array
 */
APU_DECLARE(void) apr_hook_debug_show(const char *szName,
                                      const char * const *aszPre,
                                      const char * const *aszSucc);

/** @deprecated @see apr_hook_debug_show */
APU_DECLARE(void) apr_show_hook(const char *szName,
                                const char * const *aszPre,
                                const char * const *aszSucc);

/**
 * Remove all currently registered functions.
 */
APU_DECLARE(void) apr_hook_deregister_all(void);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* APR_HOOKS_H */
