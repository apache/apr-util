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
#if APR_HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#include "ap_buckets.h"

static apr_array_header_t *bucket_types;

static apr_status_t ap_brigade_cleanup(void *data)
{
    ap_bucket_brigade *b = data;
    ap_bucket *e;

    /*
     * Bah! We can't use AP_RING_FOREACH here because this bucket has
     * gone away when we dig inside it to get the next one.
     */
    while (!AP_BRIGADE_EMPTY(b)) {
	e = AP_BRIGADE_FIRST(b);
	AP_BUCKET_REMOVE(e);
	ap_bucket_destroy(e);
    }
    /*
     * We don't need to free(bb) because it's allocated from a pool.
     */
    return APR_SUCCESS;
}
APU_DECLARE(apr_status_t) ap_brigade_destroy(ap_bucket_brigade *b)
{
    apr_kill_cleanup(b->p, b, ap_brigade_cleanup);
    return ap_brigade_cleanup(b);
}

APU_DECLARE(ap_bucket_brigade *) ap_brigade_create(apr_pool_t *p)
{
    ap_bucket_brigade *b;

    b = apr_palloc(p, sizeof(*b));
    b->p = p;
    AP_RING_INIT(&b->list, ap_bucket, link);

    apr_register_cleanup(b->p, b, ap_brigade_cleanup, ap_brigade_cleanup);
    return b;
}

APU_DECLARE(ap_bucket_brigade *) ap_brigade_split(ap_bucket_brigade *b,
						 ap_bucket *e)
{
    ap_bucket_brigade *a;
    ap_bucket *f;

    a = ap_brigade_create(b->p);
    /* Return an empty brigade if there is nothing left in 
     * the first brigade to split off 
     */
    if (e != AP_BRIGADE_SENTINEL(b)) {
        f = AP_RING_LAST(&b->list);
        AP_RING_UNSPLICE(e, f, link);
        AP_RING_SPLICE_HEAD(&a->list, e, f, ap_bucket, link);
    }
    return a;
}

APU_DECLARE(ap_bucket *) ap_brigade_partition(ap_bucket_brigade *b, apr_off_t point)
{
    ap_bucket *e;
    const char *s;
    apr_size_t len;

    if (point < 0)
        return NULL;

    AP_BRIGADE_FOREACH(e, b) {
        /* bucket is of a known length */
        if ((point > e->length) && (e->length != -1)) {
            if (AP_BUCKET_IS_EOS(e))
                return NULL;
            point -= e->length;
        }
        else if (point == e->length) {
            return AP_BUCKET_NEXT(e);
        }
        else {
            /* try to split the bucket natively */
            if (ap_bucket_split(e, point) != APR_ENOTIMPL)
                return AP_BUCKET_NEXT(e);

            /* if the bucket cannot be split, we must read from it,
             * changing its type to one that can be split */
            if (ap_bucket_read(e, &s, &len, AP_BLOCK_READ) != APR_SUCCESS)
                return NULL;

            if (point < len) {
                if (ap_bucket_split(e, point) == APR_SUCCESS)
                    return AP_BUCKET_NEXT(e);
                else
                    return NULL;
            }
            else if (point == len)
                return AP_BUCKET_NEXT(e);
            else
                point -= len;
        }
    }
    return NULL;
}

APU_DECLARE(int) ap_brigade_to_iovec(ap_bucket_brigade *b, 
				    struct iovec *vec, int nvec)
{
    ap_bucket *e;
    struct iovec *orig;
    apr_size_t iov_len;

    orig = vec;
    AP_BRIGADE_FOREACH(e, b) {
	if (nvec-- == 0)
            break;
	ap_bucket_read(e, (const char **)&vec->iov_base, &iov_len, AP_NONBLOCK_READ);
        vec->iov_len = iov_len; /* set indirectly in case size differs */
	++vec;
    }
    return vec - orig;
}

APU_DECLARE(int) ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va)
{
    ap_bucket *r;
    const char *x;
    int j, k;
    apr_size_t i;

    for (k = 0;;) {
        x = va_arg(va, const char *);
        if (x == NULL)
            break;
        j = strlen(x);
       
	/* XXX: copy or not? let the caller decide? */
        r = ap_bucket_create_heap(x, j, 1, &i);
        if (i != j) {
            /* Do we need better error reporting?  */
            return -1;
        }
        k += i;

        AP_BRIGADE_INSERT_TAIL(b, r);
    }

    return k;
}

APU_DECLARE_NONSTD(int) ap_brigade_putstrs(ap_bucket_brigade *b, ...)
{
    va_list va;
    int written;

    va_start(va, b);
    written = ap_brigade_vputstrs(b, va);
    va_end(va);
    return written;
}

APU_DECLARE_NONSTD(int) ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...)
{
    va_list ap;
    int res;

    va_start(ap, fmt);
    res = ap_brigade_vprintf(b, fmt, ap);
    va_end(ap);
    return res;
}

APU_DECLARE(int) ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va)
{
    /* XXX:  This needs to be replaced with a function to printf
     * directly into a bucket.  I'm being lazy right now.  RBB
     */
    char buf[4096];
    ap_bucket *r;
    int res;

    res = apr_vsnprintf(buf, 4096, fmt, va);

    r = ap_bucket_create_heap(buf, strlen(buf), 1, NULL);
    AP_BRIGADE_INSERT_TAIL(b, r);

    return res;
}

void ap_init_bucket_types(apr_pool_t *p)
{
    bucket_types = apr_make_array(p, 8, sizeof(ap_bucket_type));

    ap_insert_bucket_type(&ap_eos_type);
    ap_insert_bucket_type(&ap_file_type);
    ap_insert_bucket_type(&ap_heap_type);
#ifdef AP_USE_MMAP_FILES
    ap_insert_bucket_type(&ap_mmap_type);
#endif
    ap_insert_bucket_type(&ap_pipe_type);
    ap_insert_bucket_type(&ap_immortal_type);
    ap_insert_bucket_type(&ap_transient_type);
    ap_insert_bucket_type(&ap_socket_type);
}

int ap_insert_bucket_type(const ap_bucket_type *type)
{
    const ap_bucket_type **newone;

    newone = (const ap_bucket_type **)apr_push_array(bucket_types);
    newone = &type;

    return bucket_types->nelts - 1;
}

APU_DECLARE_NONSTD(apr_status_t) ap_bucket_setaside_notimpl(ap_bucket *data)
{
    return APR_ENOTIMPL;
}

APU_DECLARE_NONSTD(apr_status_t) ap_bucket_split_notimpl(ap_bucket *data, apr_off_t point)
{
    return APR_ENOTIMPL;
}
APU_DECLARE_NONSTD(apr_status_t) ap_bucket_copy_notimpl(ap_bucket *e, ap_bucket **c)
{
    return APR_ENOTIMPL;
}
APU_DECLARE_NONSTD(void) ap_bucket_destroy_notimpl(void *data)
{
    return;
}
