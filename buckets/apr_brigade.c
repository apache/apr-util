/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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
#include "apr_strings.h"
#include "apr_pools.h"
#include "apr_tables.h"
#include "apr_buckets.h"
#include "apr_errno.h"
#define APR_WANT_MEMFUNC
#define APR_WANT_STRFUNC
#include "apr_want.h"

#include <stdlib.h>
#if APR_HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

static apr_status_t brigade_cleanup(void *data) 
{
    return apr_brigade_cleanup(data);
}
APU_DECLARE(apr_status_t) apr_brigade_cleanup(void *data)
{
    apr_bucket_brigade *b = data;
    apr_bucket *e;

    /*
     * Bah! We can't use APR_RING_FOREACH here because this bucket has
     * gone away when we dig inside it to get the next one.
     */
    while (!APR_BRIGADE_EMPTY(b)) {
	e = APR_BRIGADE_FIRST(b);
	apr_bucket_delete(e);
    }
    /*
     * We don't need to free(bb) because it's allocated from a pool.
     */
    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apr_brigade_destroy(apr_bucket_brigade *b)
{
    apr_pool_cleanup_kill(b->p, b, brigade_cleanup);
    return apr_brigade_cleanup(b);
}

APU_DECLARE(apr_bucket_brigade *) apr_brigade_create(apr_pool_t *p)
{
    apr_bucket_brigade *b;

    b = apr_pcalloc(p, sizeof(*b));
    b->p = p;

    APR_RING_INIT(&b->list, apr_bucket, link);

    apr_pool_cleanup_register(b->p, b, brigade_cleanup, brigade_cleanup);
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

APU_DECLARE(apr_status_t) apr_brigade_length(apr_bucket_brigade *bb,
                                             int read_all, apr_ssize_t *length)
{
    apr_ssize_t total = 0;
    apr_bucket *bkt;

    APR_BRIGADE_FOREACH(bkt, bb) {
        if (bkt->length == -1) {
            const char *ignore;
            apr_size_t len;
            apr_status_t status;

            if (!read_all) {
                *length = -1;
                return APR_SUCCESS;
            }

            if ((status = apr_bucket_read(bkt, &ignore, &len,
                                          APR_BLOCK_READ)) != APR_SUCCESS) {
                return status;
            }
        }

        total += bkt->length;
    }

    *length = total;
    return APR_SUCCESS;
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

static int check_brigade_flush(const char **str, 
                               apr_size_t *n, apr_bucket_brigade *bb,
                               apr_brigade_flush flush)
{
    apr_bucket *b = APR_BRIGADE_LAST(bb);

    if (APR_BRIGADE_EMPTY(bb)) {
        if (*n > APR_BUCKET_BUFF_SIZE) {
            apr_bucket *e;
            if (flush) {
                e = apr_bucket_transient_create(*str, *n);
            }
            else {
                e = apr_bucket_heap_create(*str, *n, 0, NULL);
            }
            APR_BRIGADE_INSERT_TAIL(bb, e);
            return 1;
        }
        else {
            return 0;
        }
    }

    if (APR_BUCKET_IS_HEAP(b)) {
        apr_bucket_heap *h = b->data;

        if (*n > (h->alloc_len - b->length)) {
            APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_transient_create(*str, *n));
            return 1;
        }
    }
    else if (*n > APR_BUCKET_BUFF_SIZE) {
        APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_transient_create(*str, *n));
        return 1;
    }

    return 0;
}

APU_DECLARE(int) apr_brigade_vputstrs(apr_bucket_brigade *b, 
                                      apr_brigade_flush flush, void *ctx,
                                      va_list va)
{
    const char *x;
    int j, k;

    for (k = 0;;) {
        x = va_arg(va, const char *);
        if (x == NULL)
            break;
        j = strlen(x);
       
        k += apr_brigade_write(b, flush, ctx, x, j);
    }
    return k;
}

APU_DECLARE(int) apr_brigade_putc(apr_bucket_brigade *b,
                                  apr_brigade_flush flush, void *ctx,
                                  const char c)
{
    return apr_brigade_write(b, flush, ctx, &c, 1);
}

APU_DECLARE(int) apr_brigade_write(apr_bucket_brigade *bb, 
                                   apr_brigade_flush flush, void *ctx, 
                                   const char *str, apr_size_t nbyte)
{
    if (check_brigade_flush(&str, &nbyte, bb, flush)) {
        if (flush) {
            return flush(bb, ctx);
        }
    }
    else {
        apr_bucket *b = APR_BRIGADE_LAST(bb);
        char *buf;

        if (APR_BRIGADE_EMPTY(bb) || !APR_BUCKET_IS_HEAP(b)) {
            buf = malloc(APR_BUCKET_BUFF_SIZE);
            b = apr_bucket_heap_create(buf, APR_BUCKET_BUFF_SIZE, 0, NULL);
            APR_BRIGADE_INSERT_TAIL(bb, b);
            b->length = 0;   /* We are writing into the brigade, and
                              * allocating more memory than we need.  This
                              * ensures that the bucket thinks it is empty just
                              * after we create it.  We'll fix the length
                              * once we put data in it below.
                              */
        }
        else {
            apr_bucket_heap *h = b->data;
            buf = h->base + b->start + b->length;
        }

        memcpy(buf, str, nbyte);
        b->length += nbyte;
    }
    return nbyte;
}

APU_DECLARE(int) apr_brigade_puts(apr_bucket_brigade *b,
                                  apr_brigade_flush flush, void *ctx,
                                  const char *str)
{
    return apr_brigade_write(b, flush, ctx, str, strlen(str));
}

APU_DECLARE_NONSTD(int) apr_brigade_putstrs(apr_bucket_brigade *b, 
                                            apr_brigade_flush flush,
                                            void *ctx, ...)
{
    va_list va;
    int written;

    va_start(va, ctx);
    written = apr_brigade_vputstrs(b, flush, ctx, va);
    va_end(va);
    return written;
}

APU_DECLARE_NONSTD(int) apr_brigade_printf(apr_bucket_brigade *b, 
                                           apr_brigade_flush flush, void *ctx, 
                                           const char *fmt, ...)
{
    va_list ap;
    int res;

    va_start(ap, fmt);
    res = apr_brigade_vprintf(b, flush, ctx, fmt, ap);
    va_end(ap);
    return res;
}

APU_DECLARE(int) apr_brigade_vprintf(apr_bucket_brigade *b, 
                                     apr_brigade_flush flush, void *ctx, 
                                     const char *fmt, va_list va)
{
    /* XXX:  This needs to be replaced with a function to printf
     * directly into a bucket.  I'm being lazy right now.  RBB
     */
    char buf[4096];

    apr_vsnprintf(buf, sizeof(buf), fmt, va);

    return apr_brigade_puts(b, flush, ctx, buf);
} 

