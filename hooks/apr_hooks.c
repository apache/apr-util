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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "apr_pools.h"
#include "apr_tables.h"
#include "apr.h"
#include "ap_hooks.h"


#if 0
#define apr_palloc(pool,size)	malloc(size)
#endif

APU_DECLARE_DATA apr_pool_t *ap_global_hook_pool = NULL;
APU_DECLARE_DATA int ap_debug_module_hooks = 0;
APU_DECLARE_DATA const char *ap_debug_module_name = NULL;

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

static TSort *prepare(apr_pool_t *p,TSortData *pItems,int nItems)
{
    TSort *pData=apr_palloc(p,nItems*sizeof *pData);
    int n;
    
    qsort(pItems,nItems,sizeof *pItems,crude_order);
    for(n=0 ; n < nItems ; ++n) {
	pData[n].nPredecessors=0;
	pData[n].ppPredecessors=apr_pcalloc(p,nItems*sizeof *pData[n].ppPredecessors);
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

static apr_array_header_t *sort_hook(apr_array_header_t *pHooks,const char *szName)
{
    apr_pool_t *p;
    TSort *pSort;
    apr_array_header_t *pNew;
    int n;

    apr_create_pool(&p, ap_global_hook_pool);
    pSort=prepare(p,(TSortData *)pHooks->elts,pHooks->nelts);
    pSort=tsort(pSort,pHooks->nelts);
    pNew=apr_make_array(ap_global_hook_pool,pHooks->nelts,sizeof(TSortData));
    if(ap_debug_module_hooks)
	printf("Sorting %s:",szName);
    for(n=0 ; pSort ; pSort=pSort->pNext,++n) {
	TSortData *pHook;
	assert(n < pHooks->nelts);
	pHook=apr_push_array(pNew);
	memcpy(pHook,pSort->pData,sizeof *pHook);
	if(ap_debug_module_hooks)
	    printf(" %s",pHook->szName);
    }
    if(ap_debug_module_hooks)
	fputc('\n',stdout);
    return pNew;
}

static apr_array_header_t *s_aHooksToSort;
typedef struct
{
    const char *szHookName;
    apr_array_header_t **paHooks;
} HookSortEntry;

APU_DECLARE(void) ap_hook_sort_register(const char *szHookName,
                                      apr_array_header_t **paHooks)
{
    HookSortEntry *pEntry;

    if(!s_aHooksToSort)
	s_aHooksToSort=apr_make_array(ap_global_hook_pool,1,sizeof(HookSortEntry));
    pEntry=apr_push_array(s_aHooksToSort);
    pEntry->szHookName=szHookName;
    pEntry->paHooks=paHooks;
}

APU_DECLARE(void) ap_sort_hooks()
{
    int n;

    for(n=0 ; n < s_aHooksToSort->nelts ; ++n) {
	HookSortEntry *pEntry=&((HookSortEntry *)s_aHooksToSort->elts)[n];
	*pEntry->paHooks=sort_hook(*pEntry->paHooks,pEntry->szHookName);
    }
}
    
APU_DECLARE(void) ap_hook_deregister_all(void)
{
    int n;    

    for(n=0 ; n < s_aHooksToSort->nelts ; ++n) {
        HookSortEntry *pEntry=&((HookSortEntry *)s_aHooksToSort->elts)[n];
        *pEntry->paHooks=NULL;
    }
    s_aHooksToSort=NULL;
}

APU_DECLARE(void) ap_show_hook(const char *szName,const char * const *aszPre,
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
