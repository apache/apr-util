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
    array_header *link_##name;

#define IMPLEMENT_HOOK_BASE(ret,rv_decl,sv,rv,name,args,args2,run_all,term1,term2,rv_final) \
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

/* RUN_ALL runs to the first one to return other than ok or decline
   RUN_FIRST runs to the first one to return other than ok
*/
#define RUN_ALL			1
#define RUN_FIRST		0

#define IMPLEMENT_HOOK(ret,name,args,args2,run_all,ok,decline) \
	IMPLEMENT_HOOK_BASE(ret,ret r_;,r_=,r_,name,args,args2,run_all,r_ != decline,r_ != ok,run_all ? ok : decline)
#define IMPLEMENT_VOID_HOOK(name,args,args2,run_all) \
	IMPLEMENT_HOOK_BASE(void,,,,name,args,args2,run_all,1,0,)

     /* Hook orderings */
#define HOOK_REALLY_FIRST	(-10)
#define HOOK_FIRST		0
#define HOOK_MIDDLE		10
#define HOOK_LAST		20
#define HOOK_REALLY_LAST	30

extern pool *g_pHookPool;
extern int g_bDebugHooks;
extern const char *g_szCurrentHookName;

void ap_hook_sort_register(const char *szHookName,array_header **aHooks);
void ap_sort_hooks(void);
void ap_show_hook(const char *szName,const char * const *aszPre,
		  const char * const *aszSucc);

#endif /* ndef(AP_HOOKS_H) */
