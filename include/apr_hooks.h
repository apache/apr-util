#ifndef APACHE_AP_HOOKS_H
#define APACHE_AP_HOOKS_H

extern int g_bDebugHooks;

#define DECLARE_HOOK(ret,name,args) \
typedef ret HOOK_##name args; \
void ap_hook_##name(HOOK_##name *pf); \
ret ap_run_##name args; \
typedef struct _LINK_##name \
    { \
    HOOK_##name *pFunc; \
    } LINK_##name;

#define HOOK_STRUCT(members) \
static struct { members } _hooks;

#define HOOK_LINK(name) \
    array_header *link_##name;

#define IMPLEMENT_HOOK_BASE(ret,rv_decl,sv,rv,name,args,args2,run_all,term1,term2,rv_final) \
void ap_hook_##name(HOOK_##name *pf) \
    { \
    LINK_##name *pHook; \
    if(!_hooks.link_##name) \
	_hooks.link_##name=ap_make_array(g_pHookPool,1,sizeof(LINK_##name)); \
    pHook=ap_push_array(_hooks.link_##name); \
    pHook->pFunc=pf; \
    if(g_bDebugHooks) \
	puts("  Hooked " #name); \
    } \
ret ap_run_##name args \
    { \
    LINK_##name *pHook; \
    int n; \
    rv_decl \
\
    if(!_hooks.link_##name) \
	return rv_final; \
\
    pHook=(LINK_##name *)_hooks.link_##name->elts; \
    for(n=0 ; n < _hooks.link_##name->nelts ; ++n) \
	{ \
	sv pHook[n].pFunc args2; \
\
	if(term1 && (!run_all || term2)) \
	    return rv; \
	} \
    return rv_final; \
    }

#define IMPLEMENT_HOOK(ret,name,args,args2,run_all,ok,decline) \
	IMPLEMENT_HOOK_BASE(ret,ret r_;,r_=,r_,name,args,args2,run_all,r_ != decline,r_ != ok,run_all ? ok : decline)
#define IMPLEMENT_VOID_HOOK(name,args,args2,run_all) \
	IMPLEMENT_HOOK_BASE(void,,,,name,args,args2,run_all,1,0,)

extern pool *g_pHookPool;

#endif /* ndef(AP_HOOKS_H) */
