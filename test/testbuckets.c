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
#include "apr_strings.h"

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
    ABTS_ASSERT(tc, "bucket before transient is sentinel",
                APR_BUCKET_PREV(tb) == APR_BRIGADE_SENTINEL(bb));

    apr_brigade_cleanup(bb);

    ABTS_ASSERT(tc, "cleaned up brigade was empty", APR_BRIGADE_EMPTY(bb));

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

static apr_bucket_brigade *make_simple_brigade(apr_bucket_alloc_t *ba,
                                               const char *first, 
                                               const char *second)
{
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_bucket *e;
 
    e = apr_bucket_transient_create(first, strlen(first), ba);
    APR_BRIGADE_INSERT_TAIL(bb, e);

    e = apr_bucket_transient_create(second, strlen(second), ba);
    APR_BRIGADE_INSERT_TAIL(bb, e);

    return bb;
}

/* tests that 'bb' flattens to string 'expect'. */
static void flatten_match(abts_case *tc, const char *ctx,
                          apr_bucket_brigade *bb,
                          const char *expect)
{
    apr_size_t elen = strlen(expect);
    char *buf = malloc(elen);
    apr_size_t len = elen;
    char msg[200];

    sprintf(msg, "%s: flatten brigade", ctx);
    apr_assert_success(tc, msg, apr_brigade_flatten(bb, buf, &len));
    sprintf(msg, "%s: length match (%ld not %ld)", ctx,
            (long)len, (long)elen);
    ABTS_ASSERT(tc, msg, len == elen);
    sprintf(msg, "%s: result match", msg);
    ABTS_STR_NEQUAL(tc, expect, buf, len);
    free(buf);
}

static void test_flatten(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb;

    bb = make_simple_brigade(ba, "hello, ", "world");

    flatten_match(tc, "flatten brigade", bb, "hello, world");

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);    
}

static int count_buckets(apr_bucket_brigade *bb)
{
    apr_bucket *e;
    int count = 0;

    for (e = APR_BRIGADE_FIRST(bb); 
         e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e)) {
        count++;
    }
    
    return count;
}

static void test_split(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb, *bb2;
    apr_bucket *e;

    bb = make_simple_brigade(ba, "hello, ", "world");

    /* split at the "world" bucket */
    e = APR_BRIGADE_LAST(bb);
    bb2 = apr_brigade_split(bb, e);

    ABTS_ASSERT(tc, "split brigade contains one bucket",
                count_buckets(bb2) == 1);
    ABTS_ASSERT(tc, "original brigade contains one bucket",
                count_buckets(bb) == 1);

    flatten_match(tc, "match original brigade", bb, "hello, ");
    flatten_match(tc, "match split brigade", bb2, "world");

    apr_brigade_destroy(bb2);
    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

#define COUNT 3000
#define THESTR "hello"

static void test_bwrite(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_off_t length;
    int n;

    for (n = 0; n < COUNT; n++) {
        apr_assert_success(tc, "brigade_write", 
                           apr_brigade_write(bb, NULL, NULL,
                                             THESTR, sizeof THESTR));
    }

    apr_assert_success(tc, "determine brigade length",
                       apr_brigade_length(bb, 1, &length));

    ABTS_ASSERT(tc, "brigade has correct length",
                length == (COUNT * sizeof THESTR));
    
    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

static void test_splitline(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bin, *bout;

    bin = make_simple_brigade(ba, "blah blah blah-",
                              "end of line.\nfoo foo foo");
    bout = apr_brigade_create(p, ba);

    apr_assert_success(tc, "split line",
                       apr_brigade_split_line(bout, bin,
                                              APR_BLOCK_READ, 100));

    flatten_match(tc, "split line", bout, "blah blah blah-end of line.\n");
    flatten_match(tc, "remainder", bin, "foo foo foo");

    apr_brigade_destroy(bout);
    apr_brigade_destroy(bin);
    apr_bucket_alloc_destroy(ba);
}


static void test_splits(abts_case *tc, void *ctx)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb;
    apr_bucket *e;
    char *str = "alphabeta";
    int n;

    bb = apr_brigade_create(p, ba);

    APR_BRIGADE_INSERT_TAIL(bb,
                            apr_bucket_immortal_create(str, 9, ba));
    APR_BRIGADE_INSERT_TAIL(bb, 
                            apr_bucket_transient_create(str, 9, ba));
    APR_BRIGADE_INSERT_TAIL(bb, 
                            apr_bucket_heap_create(strdup(str), 9, free, ba));
    APR_BRIGADE_INSERT_TAIL(bb, 
                            apr_bucket_pool_create(apr_pstrdup(p, str), 9, p, 
                                                   ba));

    ABTS_ASSERT(tc, "four buckets inserted", count_buckets(bb) == 4);
    
    /* now split each of the buckets after byte 5 */
    for (n = 0, e = APR_BRIGADE_FIRST(bb); n < 4; n++) {
        ABTS_ASSERT(tc, "reached end of brigade", 
                    e != APR_BRIGADE_SENTINEL(bb));
        ABTS_ASSERT(tc, "split bucket OK",
                    apr_bucket_split(e, 5) == APR_SUCCESS);
        e = APR_BUCKET_NEXT(e);
        ABTS_ASSERT(tc, "split OK", e != APR_BRIGADE_SENTINEL(bb));
        e = APR_BUCKET_NEXT(e);
    }
    
    ABTS_ASSERT(tc, "four buckets split into eight", 
                count_buckets(bb) == 8);

    for (n = 0, e = APR_BRIGADE_FIRST(bb); n < 4; n++) {
        const char *data;
        apr_size_t len;
        
        apr_assert_success(tc, "read alpha from bucket",
                           apr_bucket_read(e, &data, &len, APR_BLOCK_READ));
        ABTS_ASSERT(tc, "read 5 bytes", len == 5);
        ABTS_STR_NEQUAL(tc, "alpha", data, 5);

        e = APR_BUCKET_NEXT(e);

        apr_assert_success(tc, "read beta from bucket",
                           apr_bucket_read(e, &data, &len, APR_BLOCK_READ));
        ABTS_ASSERT(tc, "read 4 bytes", len == 4);
        ABTS_STR_NEQUAL(tc, "beta", data, 5);

        e = APR_BUCKET_NEXT(e);
    }

    /* now delete the "alpha" buckets */
    for (n = 0, e = APR_BRIGADE_FIRST(bb); n < 4; n++) {
        apr_bucket *f;

        ABTS_ASSERT(tc, "reached end of brigade",
                    e != APR_BRIGADE_SENTINEL(bb));
        f = APR_BUCKET_NEXT(e);
        apr_bucket_delete(e);
        e = APR_BUCKET_NEXT(f);
    }    
    
    ABTS_ASSERT(tc, "eight buckets reduced to four", 
                count_buckets(bb) == 4);

    flatten_match(tc, "flatten beta brigade", bb,
                  "beta" "beta" "beta" "beta");

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

abts_suite *testbuckets(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_create, NULL);
    abts_run_test(suite, test_simple, NULL);
    abts_run_test(suite, test_flatten, NULL);
    abts_run_test(suite, test_split, NULL);
    abts_run_test(suite, test_bwrite, NULL);
    abts_run_test(suite, test_splitline, NULL);
    abts_run_test(suite, test_splits, NULL);

    return suite;
}


