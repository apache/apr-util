/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "abts.h"
#include "testutil.h"
#include "apr_buckets.h"

static void test_create(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    ABTS_ASSERT(tc, "new brigade not NULL", bb != NULL);
    ABTS_ASSERT(tc, "new brigade is empty", APR_BRIGADE_EMPTY(bb));

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

static void test_simple(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_bucket *fb, *tb;
    
    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);
    
    fb = APR_BRIGADE_FIRST(bb);
    ABTS_ASSERT(tc, "first bucket of empty brigade is sentinel",
                fb == APR_BRIGADE_SENTINEL(bb));

    fb = apr_bucket_flush_create(ba);
    APR_BRIGADE_INSERT_HEAD(bb, fb);

    ABTS_ASSERT(tc, "first bucket of brigade is flush",
                APR_BRIGADE_FIRST(bb) == fb);

    ABTS_ASSERT(tc, "bucket after flush is sentinel",
                APR_BUCKET_NEXT(fb) == APR_BRIGADE_SENTINEL(bb));

    tb = apr_bucket_transient_create("aaa", 3, ba);
    APR_BUCKET_INSERT_BEFORE(fb, tb);

    ABTS_ASSERT(tc, "bucket before flush now transient",
                APR_BUCKET_PREV(fb) == tb);
    ABTS_ASSERT(tc, "bucket after transient is flush",
                APR_BUCKET_NEXT(tb) == fb);

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

abts_suite *testbuckets(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_create, NULL);
    abts_run_test(suite, test_simple, NULL);

    return suite;
}


