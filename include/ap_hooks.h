#ifndef APACHE_AP_HOOKS_H
#define APACHE_AP_HOOKS_H

#define DECLARE_HOOK(ret,name,args) \
typedef ret HOOK_##name args; \
void ap_hook_##name(HOOK_##name *pf); \
ret ap_run_##name args; \
typedef struct _LINK_##name \
    { \
    HOOK_##name *pFunc; \
    struct _LINK_##name *pNext; \
    } LINK_##name;

#define HOOK_STRUCT(members) \
static struct { members } _hooks;

#define HOOK_LINK(name) \
    LINK_##name *link_##name;

#define IMPLEMENT_HOOK_BASE(ret,rv_decl,sv,rv,name,args,args2,run_all,term1,term2,rv_final) \
void ap_hook_##name(HOOK_##name *pf) \
    { \
    LINK_##name *pHook=ap_palloc(g_pHookPool,sizeof(LINK_##name)); \
    pHook->pNext=_hooks.link_##name; \
    pHook->pFunc=pf; \
    _hooks.link_##name=pHook; \
    } \
ret ap_run_##name args \
    { \
    LINK_##name *pHook; \
    rv_decl \
\
    for(pHook=_hooks.link_##name ; pHook ; pHook=pHook->pNext) \
	{ \
	sv pHook->pFunc args2; \
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
