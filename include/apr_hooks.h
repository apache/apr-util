/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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

#ifndef APACHE_AP_HOOKS_H
#define APACHE_AP_HOOKS_H

/* For ap_array_header_t */
#include "apr_lib.h"

#define AP_DECLARE_HOOK(ret,name,args) \
typedef ret HOOK_##name args; \
void ap_hook_##name(HOOK_##name *pf,const char * const *aszPre, \
		    const char * const *aszSucc,int nOrder); \
ret ap_run_##name args; \
typedef struct _LINK_##name \
    { \
    HOOK_##name *pFunc; \
    const char *szName; \
    const char * const *aszPredecessors; \
    const char * const *aszSuccessors; \
    int nOrder; \
    } LINK_##name;

#define AP_HOOK_STRUCT(members) \
static struct { members } _hooks;

#define AP_HOOK_LINK(name) \
    ap_array_header_t *link_##name;

#define AP_IMPLEMENT_HOOK_BASE(name) \
void ap_hook_##name(HOOK_##name *pf,const char * const *aszPre, \
		    const char * const *aszSucc,int nOrder) \
    { \
    LINK_##name *pHook; \
    if(!_hooks.link_##name) \
	{ \
	_hooks.link_##name=ap_make_array(g_pHookPool,1,sizeof(LINK_##name)); \
	ap_hook_sort_register(#name,&_hooks.link_##name); \
	} \
    pHook=ap_push_array(_hooks.link_##name); \
    pHook->pFunc=pf; \
    pHook->aszPredecessors=aszPre; \
    pHook->aszSuccessors=aszSucc; \
    pHook->nOrder=nOrder; \
    pHook->szName=g_szCurrentHookName; \
    if(g_bDebugHooks) \
	ap_show_hook(#name,aszPre,aszSucc); \
    }

/* RUN_ALL runs to the first one to return other than ok or decline
   RUN_FIRST runs to the first one to return other than decline
   VOID runs all
*/

#define AP_IMPLEMENT_HOOK_VOID(name,args_decl,args_use) \
AP_IMPLEMENT_HOOK_BASE(name) \
void ap_run_##name args_decl \
    { \
    LINK_##name *pHook; \
    int n; \
\
    if(!_hooks.link_##name) \
	return; \
\
    pHook=(LINK_##name *)_hooks.link_##name->elts; \
    for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	pHook[n].pFunc args_use; \
    }

/* FIXME: note that this returns ok when nothing is run. I suspect it should
   really return decline, but that breaks Apache currently - Ben
*/
#define AP_IMPLEMENT_HOOK_RUN_ALL(ret,name,args_decl,args_use,ok,decline) \
AP_IMPLEMENT_HOOK_BASE(name) \
ret ap_run_##name args_decl \
    { \
    LINK_##name *pHook; \
    int n; \
    ret rv; \
\
    if(!_hooks.link_##name) \
	return ok; \
\
    pHook=(LINK_##name *)_hooks.link_##name->elts; \
    for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	{ \
	rv=pHook[n].pFunc args_use; \
\
	if(rv != ok && rv != decline) \
	    return rv; \
	} \
    return ok; \
    }

#define AP_IMPLEMENT_HOOK_RUN_FIRST(ret,name,args_decl,args_use,decline) \
AP_IMPLEMENT_HOOK_BASE(name) \
ret ap_run_##name args_decl \
    { \
    LINK_##name *pHook; \
    int n; \
    ret rv; \
\
    if(!_hooks.link_##name) \
	return decline; \
\
    pHook=(LINK_##name *)_hooks.link_##name->elts; \
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
#define AP_HOOK_REALLY_FIRST	(-10)
#define AP_HOOK_FIRST		0
#define AP_HOOK_MIDDLE		10
#define AP_HOOK_LAST		20
#define AP_HOOK_REALLY_LAST	30

extern ap_pool_t *g_pHookPool;
extern int g_bDebugHooks;
extern const char *g_szCurrentHookName;

void ap_hook_sort_register(const char *szHookName, ap_array_header_t **aHooks);
void ap_sort_hooks(void);
void ap_show_hook(const char *szName,const char * const *aszPre,
		  const char * const *aszSucc);

#endif /* ndef(AP_HOOKS_H) */
