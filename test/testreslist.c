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

#include <stdio.h>
#include <stdlib.h>
#include "apr_reslist.h"
#include "apr_thread_proc.h"

#if !APR_HAS_THREADS

int main(void)
{
    fprintf(stderr, "this program requires APR thread support\n");
    return 0;
}

#else

#define RESLIST_MIN   3
#define RESLIST_SMAX 10
#define RESLIST_HMAX 20
#define RESLIST_TTL  APR_TIME_C(350000) /* 35 ms */
#define CONSUMER_THREADS 25
#define CONSUMER_ITERATIONS 250
#define CONSTRUCT_SLEEP_TIME  APR_TIME_C(250000) /* 25 ms */
#define DESTRUCT_SLEEP_TIME   APR_TIME_C(100000) /* 10 ms */
#define WORK_DELAY_SLEEP_TIME APR_TIME_C(150000) /* 15 ms */

typedef struct {
    apr_interval_time_t sleep_upon_construct;
    apr_interval_time_t sleep_upon_destruct;
    int c_count;
    int d_count;
} my_parameters_t;

typedef struct {
    int id;
} my_resource_t;

static apr_status_t my_constructor(void **resource, void *params,
                                   apr_pool_t *pool)
{
    my_resource_t *res;
    my_parameters_t *my_params = params;

    /* Create some resource */
    res = apr_palloc(pool, sizeof(*res));
    res->id = my_params->c_count++;

    printf("++ constructing new resource [id:%d]\n", res->id);

    /* Sleep for awhile, to simulate construction overhead. */
    apr_sleep(my_params->sleep_upon_construct);

    /* Set the resource so it can be managed by the reslist */
    *resource = res;
    return APR_SUCCESS;
}

static apr_status_t my_destructor(void *resource, void *params,
                                  apr_pool_t *pool)
{
    my_resource_t *res = resource;
    my_parameters_t *my_params = params;

    printf("-- destructing old resource [id:%d, #%d]\n", res->id,
           my_params->d_count++);

    apr_sleep(my_params->sleep_upon_destruct);

    return APR_SUCCESS;
}

typedef struct {
    int tid;
    apr_reslist_t *reslist;
    apr_interval_time_t work_delay_sleep;
} my_thread_info_t;

static void * APR_THREAD_FUNC resource_consuming_thread(apr_thread_t *thd,
                                                        void *data)
{
    apr_status_t rv;
    my_thread_info_t *thread_info = data;
    apr_reslist_t *rl = thread_info->reslist;
    int i;

    for (i = 0; i < CONSUMER_ITERATIONS; i++) {
        my_resource_t *res;
        rv = apr_reslist_acquire(rl, (void**)&res);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "Failed to retrieve resource from reslist\n");
            apr_thread_exit(thd, rv);
            return NULL;
        }
        printf("  [tid:%d,iter:%d] using resource id:%d\n", thread_info->tid,
               i, res->id);
        apr_sleep(thread_info->work_delay_sleep);
        rv = apr_reslist_release(rl, res);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "Failed to return resource to reslist\n");
            apr_thread_exit(thd, rv);
            return NULL;
        }
    }

    return APR_SUCCESS;
}

static apr_status_t test_reslist(apr_pool_t *parpool)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_reslist_t *rl;
    my_parameters_t *params;
    int i;
    apr_thread_t *my_threads[CONSUMER_THREADS];
    my_thread_info_t my_thread_info[CONSUMER_THREADS];

    printf("Creating child pool.......................");
    rv = apr_pool_create(&pool, parpool);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Error creating child pool\n");
        return rv;
    }
    printf("OK\n");

    /* Create some parameters that will be passed into each
     * constructor and destructor call. */
    params = apr_pcalloc(pool, sizeof(*params));
    params->sleep_upon_construct = CONSTRUCT_SLEEP_TIME;
    params->sleep_upon_destruct = DESTRUCT_SLEEP_TIME;

    /* We're going to want 10 blocks of data from our target rmm. */
    printf("Creating resource list:\n"
           " min/smax/hmax: %d/%d/%d\n"
           " ttl: %" APR_TIME_T_FMT "\n", RESLIST_MIN, RESLIST_SMAX,
           RESLIST_HMAX, RESLIST_TTL);
    rv = apr_reslist_create(&rl, RESLIST_MIN, RESLIST_SMAX, RESLIST_HMAX,
                            RESLIST_TTL, my_constructor, my_destructor,
                            params, pool);
    if (rv != APR_SUCCESS) { 
        fprintf(stderr, "Error allocating shared memory block\n");
        return rv;
    }
    fprintf(stdout, "OK\n");

    printf("Creating %d threads", CONSUMER_THREADS);
    for (i = 0; i < CONSUMER_THREADS; i++) {
        putchar('.');
        my_thread_info[i].tid = i;
        my_thread_info[i].reslist = rl;
        my_thread_info[i].work_delay_sleep = WORK_DELAY_SLEEP_TIME;
        rv = apr_thread_create(&my_threads[i], NULL,
                               resource_consuming_thread, &my_thread_info[i],
                               pool);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return rv;
        }
    }
    printf("\nDone!\n");

    printf("Waiting for threads to finish");
    for (i = 0; i < CONSUMER_THREADS; i++) {
        apr_status_t thread_rv;
        putchar('.');
        apr_thread_join(&thread_rv, my_threads[i]);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "Failed to join thread %d\n", i);
            return rv;
        }
    }
    printf("\nDone!\n");

    printf("Destroying resource list.................");
    rv = apr_reslist_destroy(rl);
    if (rv != APR_SUCCESS) {
        printf("FAILED\n");
        return rv;
    }
    printf("OK\n");

    apr_pool_destroy(pool);

    return APR_SUCCESS;
}


int main(void)
{
    apr_status_t rv;
    apr_pool_t *pool;
    char errmsg[200];

    apr_initialize();
    
    printf("APR Resource List Test\n");
    printf("======================\n\n");

    printf("Initializing the pool............................"); 
    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        printf("could not initialize pool\n");
        exit(-1);
    }
    printf("OK\n");

    rv = test_reslist(pool);
    if (rv != APR_SUCCESS) {
        printf("Resource list test FAILED: [%d] %s\n",
               rv, apr_strerror(rv, errmsg, sizeof(errmsg)));
        exit(-2);
    }
    printf("Resource list test passed!\n");

    return 0;
}

#endif /* APR_HAS_THREADS */
