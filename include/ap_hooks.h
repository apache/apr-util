#ifndef APACHE_AP_HOOKS_H
#define APACHE_AP_HOOKS_H

#define DECLARE_HOOK(ret,name,args) \
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

#define HOOK_STRUCT(members) \
static struct { members } _hooks;

#define HOOK_LINK(name) \
    ap_array_header_t *link_##name;

#define IMPLEMENT_HOOK_BASE(name) \
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

#define IMPLEMENT_HOOK_VOID(name,args_decl,args_use) \
IMPLEMENT_HOOK_BASE(name) \
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
#define IMPLEMENT_HOOK_RUN_ALL(ret,name,args_decl,args_use,ok,decline) \
IMPLEMENT_HOOK_BASE(name) \
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

#define IMPLEMENT_HOOK_RUN_FIRST(ret,name,args_decl,args_use,decline) \
IMPLEMENT_HOOK_BASE(name) \
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
#define HOOK_REALLY_FIRST	(-10)
#define HOOK_FIRST		0
#define HOOK_MIDDLE		10
#define HOOK_LAST		20
#define HOOK_REALLY_LAST	30

extern ap_context_t *g_pHookPool;
extern int g_bDebugHooks;
extern const char *g_szCurrentHookName;

void ap_hook_sort_register(const char *szHookName, ap_array_header_t **aHooks);
void ap_sort_hooks(void);
void ap_show_hook(const char *szName,const char * const *aszPre,
		  const char * const *aszSucc);

#endif /* ndef(AP_HOOKS_H) */
