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

#include <assert.h>

#include "apu.h"
#include "apr_reslist.h"
#include "apr_errno.h"
#include "apr_strings.h"
#include "apr_thread_mutex.h"
#include "apr_thread_cond.h"
#include "apr_ring.h"

#if APR_HAS_THREADS

/**
 * A single resource element.
 */
struct apr_res_t {
    apr_time_t freed;
    void *opaque;
    APR_RING_ENTRY(apr_res_t) link;
};
typedef struct apr_res_t apr_res_t;

/**
 * A ring of resources representing the list of available resources.
 */
APR_RING_HEAD(apr_resring_t, apr_res_t);
typedef struct apr_resring_t apr_resring_t;

struct apr_reslist_t {
    apr_pool_t *pool; /* the pool used in constructor and destructor calls */
    int ntotal;     /* total number of resources managed by this list */
    int nidle;      /* number of available resources */
    int min;  /* desired minimum number of available resources */
    int smax; /* soft maximum on the total number of resources */
    int hmax; /* hard maximum on the total number of resources */
    apr_interval_time_t ttl; /* TTL when we have too many resources */
    apr_reslist_constructor constructor;
    apr_reslist_destructor destructor;
    void *params; /* opaque data passed to constructor and destructor calls */
    apr_resring_t avail_list;
    apr_resring_t free_list;
    apr_thread_mutex_t *listlock;
    apr_thread_cond_t *avail;
};

/**
 * Grab a resource from the front of the resource list.
 * Assumes: that the reslist is locked.
 */
static apr_res_t *pop_resource(apr_reslist_t *reslist)
{
    apr_res_t *res;
    res = APR_RING_FIRST(&reslist->avail_list);
    APR_RING_REMOVE(res, link);
    reslist->nidle--;
    return res;
}

/**
 * Add a resource to the end of the list, set the time at which
 * it was added to the list.
 * Assumes: that the reslist is locked.
 */
static void push_resource(apr_reslist_t *reslist, apr_res_t *resource)
{
    APR_RING_INSERT_TAIL(&reslist->avail_list, resource, apr_res_t, link);
    resource->freed = apr_time_now();
    reslist->nidle++;
}

/**
 * Get an empty resource container from the free list.
 */
static apr_res_t *get_container(apr_reslist_t *reslist)
{
    apr_res_t *res;

    assert(!APR_RING_EMPTY(&reslist->free_list, apr_res_t, link));

    res = APR_RING_FIRST(&reslist->free_list);
    APR_RING_REMOVE(res, link);

    return res;
}

/**
 * Free up a resource container by placing it on the free list.
 */
static void free_container(apr_reslist_t *reslist, apr_res_t *container)
{
    APR_RING_INSERT_TAIL(&reslist->free_list, container, apr_res_t, link);
}

/**
 * Create a new resource and return it.
 * Assumes: that the reslist is locked.
 */
static apr_status_t create_resource(apr_reslist_t *reslist, apr_res_t **ret_res)
{
    apr_status_t rv;
    apr_res_t *res;

    res = apr_pcalloc(reslist->pool, sizeof(*res));

    rv = reslist->constructor(&res->opaque, reslist->params, reslist->pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    *ret_res = res;
    return APR_SUCCESS;
}

/**
 * Destroy a single idle resource.
 * Assumes: that the reslist is locked.
 */
static apr_status_t destroy_resource(apr_reslist_t *reslist, apr_res_t *res)
{
    apr_status_t rv;

    rv = reslist->destructor(res->opaque, reslist->params, reslist->pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    return APR_SUCCESS;
}

static apr_status_t reslist_cleanup(void *data_)
{
    apr_status_t rv;
    apr_reslist_t *rl = data_;
    apr_res_t *res;

    apr_thread_mutex_lock(rl->listlock);

    while (rl->nidle > 0) {
        res = pop_resource(rl);
        rl->ntotal--;
        rv = destroy_resource(rl, res);
        if (rv != APR_SUCCESS) {
            return rv;
        }
        free_container(rl, res);
    }

    assert(rl->nidle == 0);
    assert(rl->ntotal == 0);

    apr_thread_mutex_destroy(rl->listlock);
    apr_thread_cond_destroy(rl->avail);

    return APR_SUCCESS;
}

/**
 * Perform routine maintenance on the resource list. This call
 * may instantiate new resources or expire old resources.
 */
static apr_status_t reslist_maint(apr_reslist_t *reslist)
{
    apr_time_t now;
    apr_status_t rv;
    apr_res_t *res;
    int created_one = 0;

    apr_thread_mutex_lock(reslist->listlock);

    /* Check if we need to create more resources, and if we are allowed to. */
    while (reslist->nidle < reslist->min && reslist->ntotal <= reslist->hmax) {
        /* Create the resource */
        rv = create_resource(reslist, &res);
        if (rv != APR_SUCCESS) {
            apr_thread_mutex_unlock(reslist->listlock);
            return rv;
        }
        /* Add it to the list */
        push_resource(reslist, res);
        /* Update our counters */
        reslist->ntotal++;
        /* If someone is waiting on that guy, wake them up. */
        rv = apr_thread_cond_signal(reslist->avail);
        if (rv != APR_SUCCESS) {
            apr_thread_mutex_unlock(reslist->listlock);
            return rv;
        }
        created_one++;
    }

    /* We don't need to see if we're over the max if we were under it before */
    if (created_one) {
        apr_thread_mutex_unlock(reslist->listlock);
        return APR_SUCCESS;
    }

    /* Check if we need to expire old resources */
    now = apr_time_now();
    while (reslist->nidle > reslist->smax && reslist->nidle > 0) {
        /* Peak at the first resource in the list */
        res = APR_RING_FIRST(&reslist->avail_list);
        /* See if the oldest entry should be expired */
        if (now - res->freed < reslist->ttl) {
            /* If this entry is too young, none of the others
             * will be ready to be expired either, so we are done. */
            break;
        }
        res = pop_resource(reslist);
        reslist->ntotal--;
        rv = destroy_resource(reslist, res);
        if (rv != APR_SUCCESS) {
            apr_thread_mutex_unlock(reslist->listlock);
            return rv;
        }
        free_container(reslist, res);
    }

    apr_thread_mutex_unlock(reslist->listlock);
    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apr_reslist_create(apr_reslist_t **reslist,
                                             int min, int smax, int hmax,
                                             apr_interval_time_t ttl,
                                             apr_reslist_constructor con,
                                             apr_reslist_destructor de,
                                             void *params,
                                             apr_pool_t *pool)
{
    apr_status_t rv;
    apr_reslist_t *rl;

    /* Do some sanity checks so we don't thrash around in the
     * maintenance routine later. */
    if (min > smax || min > hmax || smax > hmax || ttl < 0) {
        return APR_EINVAL;
    }

    rl = apr_pcalloc(pool, sizeof(*rl));
    rl->pool = pool;
    rl->min = min;
    rl->smax = smax;
    rl->hmax = hmax;
    rl->ttl = ttl;
    rl->constructor = con;
    rl->destructor = de;
    rl->params = params;

    APR_RING_INIT(&rl->avail_list, apr_res_t, link);
    APR_RING_INIT(&rl->free_list, apr_res_t, link);

    rv = apr_thread_mutex_create(&rl->listlock, APR_THREAD_MUTEX_DEFAULT,
                                 pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }
    rv = apr_thread_cond_create(&rl->avail, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = reslist_maint(rl);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    apr_pool_cleanup_register(rl->pool, rl, reslist_cleanup,
                              apr_pool_cleanup_null);

    *reslist = rl;

    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apr_reslist_destroy(apr_reslist_t *reslist)
{
    return apr_pool_cleanup_run(reslist->pool, reslist, reslist_cleanup);
}

APU_DECLARE(apr_status_t) apr_reslist_acquire(apr_reslist_t *reslist,
                                              void **resource)
{
    apr_status_t rv;
    apr_res_t *res;

    apr_thread_mutex_lock(reslist->listlock);
    /* If there are idle resources on the available list, use
     * them right away. */
    if (reslist->nidle > 0) {
        /* Pop off the first resource */
        res = pop_resource(reslist);
        *resource = res->opaque;
        free_container(reslist, res);
        apr_thread_mutex_unlock(reslist->listlock);
        return APR_SUCCESS;
    }
    /* If we've hit our max, block until we're allowed to create
     * a new one, or something becomes free. */
    else while (reslist->ntotal >= reslist->hmax
                && reslist->nidle <= 0) {
        apr_thread_cond_wait(reslist->avail, reslist->listlock);
    }
    /* If we popped out of the loop, first try to see if there
     * are new resources available for immediate use. */
    if (reslist->nidle > 0) {
        res = pop_resource(reslist);
        *resource = res->opaque;
        free_container(reslist, res);
        apr_thread_mutex_unlock(reslist->listlock);
        return APR_SUCCESS;
    }
    /* Otherwise the reason we dropped out of the loop
     * was because there is a new slot available, so create
     * a resource to fill the slot and use it. */
    else {
        rv = create_resource(reslist, &res);

        if (rv != APR_SUCCESS) {
           apr_thread_mutex_unlock(reslist->listlock);
           return rv;
        }

        reslist->ntotal++;
        *resource = res->opaque;
        free_container(reslist, res);
        apr_thread_mutex_unlock(reslist->listlock);
        return APR_SUCCESS;
    }
}

APU_DECLARE(apr_status_t) apr_reslist_release(apr_reslist_t *reslist,
                                              void *resource)
{
    apr_res_t *res;

    apr_thread_mutex_lock(reslist->listlock);
    res = get_container(reslist);
    res->opaque = resource;
    push_resource(reslist, res);
    apr_thread_cond_signal(reslist->avail);
    apr_thread_mutex_unlock(reslist->listlock);

    return reslist_maint(reslist);
}

#endif  /* APR_HAS_THREADS */
