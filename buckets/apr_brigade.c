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

#include "apr.h"
#include "apr_lib.h"
#include "apr_pools.h"
#include "apr_tables.h"
#include "apr_errno.h"

#include <stdlib.h>
#if APR_HAVE_STRINGS_H
#include <strings.h>
#endif
#if APR_HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#include "apr_buckets.h"

static apr_status_t apr_brigade_cleanup(void *data)
{
    apr_bucket_brigade *b = data;
    apr_bucket *e;

    /*
     * Bah! We can't use APR_RING_FOREACH here because this bucket has
     * gone away when we dig inside it to get the next one.
     */
    while (!APR_BRIGADE_EMPTY(b)) {
	e = APR_BRIGADE_FIRST(b);
	APR_BUCKET_REMOVE(e);
	apr_bucket_destroy(e);
    }
    /*
     * We don't need to free(bb) because it's allocated from a pool.
     */
    return APR_SUCCESS;
}
APU_DECLARE(apr_status_t) apr_brigade_destroy(apr_bucket_brigade *b)
{
    apr_kill_cleanup(b->p, b, apr_brigade_cleanup);
    return apr_brigade_cleanup(b);
}

APU_DECLARE(apr_bucket_brigade *) apr_brigade_create(apr_pool_t *p)
{
    apr_bucket_brigade *b;

    b = apr_palloc(p, sizeof(*b));
    b->p = p;
    APR_RING_INIT(&b->list, apr_bucket, link);

    apr_register_cleanup(b->p, b, apr_brigade_cleanup, apr_brigade_cleanup);
    return b;
}

APU_DECLARE(apr_bucket_brigade *) apr_brigade_split(apr_bucket_brigade *b,
						 apr_bucket *e)
{
    apr_bucket_brigade *a;
    apr_bucket *f;

    a = apr_brigade_create(b->p);
    /* Return an empty brigade if there is nothing left in 
     * the first brigade to split off 
     */
    if (e != APR_BRIGADE_SENTINEL(b)) {
        f = APR_RING_LAST(&b->list);
        APR_RING_UNSPLICE(e, f, link);
        APR_RING_SPLICE_HEAD(&a->list, e, f, apr_bucket, link);
    }
    return a;
}

APU_DECLARE(apr_bucket *) apr_brigade_partition(apr_bucket_brigade *b, apr_off_t point)
{
    apr_bucket *e;
    const char *s;
    apr_size_t len;

    if (point < 0)
        return NULL;

    APR_BRIGADE_FOREACH(e, b) {
        /* bucket is of a known length */
        if ((point > e->length) && (e->length != -1)) {
            if (APR_BUCKET_IS_EOS(e))
                return NULL;
            point -= e->length;
        }
        else if (point == e->length) {
            return APR_BUCKET_NEXT(e);
        }
        else {
            /* try to split the bucket natively */
            if (apr_bucket_split(e, point) != APR_ENOTIMPL)
                return APR_BUCKET_NEXT(e);

            /* if the bucket cannot be split, we must read from it,
             * changing its type to one that can be split */
            if (apr_bucket_read(e, &s, &len, APR_BLOCK_READ) != APR_SUCCESS)
                return NULL;

            if (point < len) {
                if (apr_bucket_split(e, point) == APR_SUCCESS)
                    return APR_BUCKET_NEXT(e);
                else
                    return NULL;
            }
            else if (point == len)
                return APR_BUCKET_NEXT(e);
            else
                point -= len;
        }
    }
    return NULL;
}

APU_DECLARE(int) apr_brigade_to_iovec(apr_bucket_brigade *b, 
				    struct iovec *vec, int nvec)
{
    apr_bucket *e;
    struct iovec *orig;
    apr_size_t iov_len;

    orig = vec;
    APR_BRIGADE_FOREACH(e, b) {
	if (nvec-- == 0)
            break;
	apr_bucket_read(e, (const char **)&vec->iov_base, &iov_len, APR_NONBLOCK_READ);
        vec->iov_len = iov_len; /* set indirectly in case size differs */
	++vec;
    }
    return vec - orig;
}

APU_DECLARE(int) apr_brigade_vputstrs(apr_bucket_brigade *b, va_list va)
{
    apr_bucket *r;
    const char *x;
    int j, k;
    apr_size_t i;

    for (k = 0;;) {
        x = va_arg(va, const char *);
        if (x == NULL)
            break;
        j = strlen(x);
       
	/* XXX: copy or not? let the caller decide? */
        r = apr_bucket_create_heap(x, j, 1, &i);
        if (i != j) {
            /* Do we need better error reporting?  */
            return -1;
        }
        k += i;

        APR_BRIGADE_INSERT_TAIL(b, r);
    }

    return k;
}

APU_DECLARE_NONSTD(int) apr_brigade_putstrs(apr_bucket_brigade *b, ...)
{
    va_list va;
    int written;

    va_start(va, b);
    written = apr_brigade_vputstrs(b, va);
    va_end(va);
    return written;
}

APU_DECLARE_NONSTD(int) apr_brigade_printf(apr_bucket_brigade *b, const char *fmt, ...)
{
    va_list ap;
    int res;

    va_start(ap, fmt);
    res = apr_brigade_vprintf(b, fmt, ap);
    va_end(ap);
    return res;
}

APU_DECLARE(int) apr_brigade_vprintf(apr_bucket_brigade *b, const char *fmt, va_list va)
{
    /* XXX:  This needs to be replaced with a function to printf
     * directly into a bucket.  I'm being lazy right now.  RBB
     */
    char buf[4096];
    apr_bucket *r;
    int res;

    res = apr_vsnprintf(buf, 4096, fmt, va);

    r = apr_bucket_create_heap(buf, strlen(buf), 1, NULL);
    APR_BRIGADE_INSERT_TAIL(b, r);

    return res;
}
