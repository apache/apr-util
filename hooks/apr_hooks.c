#include "httpd.h"
#include "ap_hooks.h"
#include <assert.h>
#include <stdlib.h>

#if 0
#define ap_palloc(pool,size)	malloc(size)
#endif

/* NB: This must echo the LINK_##name structure */
typedef struct
{
    void (*dummy)(void *);
    const char *szName;
    const char * const *aszPredecessors;
    const char * const *aszSuccessors;
    int nOrder;
} TSortData;

typedef struct tsort_
{
    void *pData;
    int nPredecessors;
    struct tsort_ **ppPredecessors;
    struct tsort_ *pNext;
} TSort;

static int crude_order(const void *a_,const void *b_)
{
    const TSortData *a=a_;
    const TSortData *b=b_;

    return a->nOrder-b->nOrder;
}

static TSort *prepare(ap_context_t *p,TSortData *pItems,int nItems)
{
    TSort *pData=ap_palloc(p,nItems*sizeof *pData);
    int n;
    
    qsort(pItems,nItems,sizeof *pItems,crude_order);
    for(n=0 ; n < nItems ; ++n) {
	pData[n].nPredecessors=0;
	pData[n].ppPredecessors=ap_palloc(p,nItems*sizeof *pData[n].ppPredecessors);
	pData[n].pNext=NULL;
	pData[n].pData=&pItems[n];
    }

    for(n=0 ; n < nItems ; ++n) {
	int i,k;

	for(i=0 ; pItems[n].aszPredecessors && pItems[n].aszPredecessors[i] ; ++i)
	    for(k=0 ; k < nItems ; ++k)
		if(!strcmp(pItems[k].szName,pItems[n].aszPredecessors[i])) {
		    int l;

		    for(l=0 ; l < pData[n].nPredecessors ; ++l)
			if(pData[n].ppPredecessors[l] == &pData[k])
			    goto got_it;
		    pData[n].ppPredecessors[pData[n].nPredecessors]=&pData[k];
		    ++pData[n].nPredecessors;
		got_it:
		    break;
		}
	for(i=0 ; pItems[n].aszSuccessors && pItems[n].aszSuccessors[i] ; ++i)
	    for(k=0 ; k < nItems ; ++k)
		if(!strcmp(pItems[k].szName,pItems[n].aszSuccessors[i])) {
		    int l;

		    for(l=0 ; l < pData[k].nPredecessors ; ++l)
			if(pData[k].ppPredecessors[l] == &pData[n])
			    goto got_it2;
		    pData[k].ppPredecessors[pData[k].nPredecessors]=&pData[n];
		    ++pData[k].nPredecessors;
		got_it2:
		    break;
		}
    }

    return pData;
}

static TSort *tsort(TSort *pData,int nItems)
{
    int nTotal;
    TSort *pHead=NULL;
    TSort *pTail=NULL;

    for(nTotal=0 ; nTotal < nItems ; ++nTotal) {
	int n,i,k;

	for(n=0 ; ; ++n) {
	    if(n == nItems)
		assert(0);      /* // we have a loop... */
	    if(!pData[n].pNext && !pData[n].nPredecessors)
		break;
	}
	if(pTail)
	    pTail->pNext=&pData[n];
	else
	    pHead=&pData[n];
	pTail=&pData[n];
	pTail->pNext=pTail;     /* // fudge it so it looks linked */
	for(i=0 ; i < nItems ; ++i)
	    for(k=0 ; pData[i].ppPredecessors[k] ; ++k)
		if(pData[i].ppPredecessors[k] == &pData[n]) {
		    --pData[i].nPredecessors;
		    break;
		}
    }
    pTail->pNext=NULL;  /* // unfudge the tail */
    return pHead;
}

static ap_array_header_t *sort_hook(ap_array_header_t *pHooks,const char *szName)
{
    ap_context_t *p;
    TSort *pSort;
    ap_array_header_t *pNew;
    int n;

    ap_create_context(&p, g_pHookPool);
    pSort=prepare(p,(TSortData *)pHooks->elts,pHooks->nelts);
    pSort=tsort(pSort,pHooks->nelts);
    pNew=ap_make_array(g_pHookPool,pHooks->nelts,sizeof(TSortData));
    if(g_bDebugHooks)
	printf("Sorting %s:",szName);
    for(n=0 ; pSort ; pSort=pSort->pNext,++n) {
	TSortData *pHook;
	assert(n < pHooks->nelts);
	pHook=ap_push_array(pNew);
	memcpy(pHook,pSort->pData,sizeof *pHook);
	if(g_bDebugHooks)
	    printf(" %s",pHook->szName);
    }
    if(g_bDebugHooks)
	fputc('\n',stdout);
    return pNew;
}

static ap_array_header_t *s_aHooksToSort;
typedef struct
{
    const char *szHookName;
    ap_array_header_t **paHooks;
} HookSortEntry;

void ap_hook_sort_register(const char *szHookName,ap_array_header_t **paHooks)
{
    HookSortEntry *pEntry;

    if(!s_aHooksToSort)
	s_aHooksToSort=ap_make_array(g_pHookPool,1,sizeof(HookSortEntry));
    pEntry=ap_push_array(s_aHooksToSort);
    pEntry->szHookName=szHookName;
    pEntry->paHooks=paHooks;
}

void ap_sort_hooks()
{
    int n;

    for(n=0 ; n < s_aHooksToSort->nelts ; ++n) {
	HookSortEntry *pEntry=&((HookSortEntry *)s_aHooksToSort->elts)[n];
	*pEntry->paHooks=sort_hook(*pEntry->paHooks,pEntry->szHookName);
    }
}

void ap_show_hook(const char *szName,const char * const *aszPre,
		  const char * const *aszSucc)
{
    int nFirst;

    printf("  Hooked %s",szName);
    if(aszPre) {
	fputs(" pre(",stdout);
	nFirst=1;
	while(*aszPre) {
	    if(!nFirst)
		fputc(',',stdout);
	    nFirst=0;
	    fputs(*aszPre,stdout);
	    ++aszPre;
	}
	fputc(')',stdout);
    }
    if(aszSucc) {
	fputs(" succ(",stdout);
	nFirst=1;
	while(*aszSucc) {
	    if(!nFirst)
		fputc(',',stdout);
	    nFirst=0;
	    fputs(*aszSucc,stdout);
	    ++aszSucc;
	}
	fputc(')',stdout);
    }
    fputc('\n',stdout);
}

#if 0
void main()
{
    const char *aszAPre[]={"b","c",NULL};
    const char *aszBPost[]={"a",NULL};
    const char *aszCPost[]={"b",NULL};
    TSortData t1[]=
    {
	{ "a",aszAPre,NULL },
	{ "b",NULL,aszBPost },
	{ "c",NULL,aszCPost }
    };
    TSort *pResult;

    pResult=prepare(t1,3);
    pResult=tsort(pResult,3);

    for( ; pResult ; pResult=pResult->pNext)
	printf("%s\n",pResult->pData->szName);
}
#endif
