#define DECLARE_HOOK(ret,name,args) \
typedef ret HOOK_##name args; \
void hook_##name(HOOK_##name *pf); \
typedef struct _LINK_##name \
    { \
    HOOK_##name *pFunc; \
    struct _LINK_##name *pNext; \
    } LINK_##name;

#define HOOK_STRUCT(members) \
static struct { members } _hooks;

#define HOOK_LINK(name) \
    LINK_##name *link_##name;

#define IMPLEMENT_HOOK_BASE(ret,rv_decl,sv,rv,name,args,args2,run_all,terminate) \
void hook_##name(HOOK_##name *pf) \
    { \
    LINK_##name *pHook=ap_palloc(g_pHookPool,sizeof(LINK_##name)); \
    pHook->pNext=_hooks.link_##name; \
    pHook->pFunc=pf; \
    _hooks.link_##name=pHook; \
    } \
ret run_##name args \
    { \
    LINK_##name *pHook; \
    rv_decl \
\
    for(pHook=_hooks.link_##name ; pHook ; pHook=pHook->pNext) \
	{ \
	sv pHook->pFunc args2; \
\
	if(!run_all terminate) \
	    return rv; \
	} \
    return rv; \
    }

#define IMPLEMENT_HOOK(ret,name,args,args2,run_all,finish) \
	IMPLEMENT_HOOK_BASE(ret,ret r;,r=,r,name,args,args2,run_all,&& r == finish)
#define IMPLEMENT_VOID_HOOK(name,args,args2,run_all) \
	IMPLEMENT_HOOK_BASE(void,,,,name,args,args2,run_all,)

extern pool *g_pHookPool;
