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

#include "httpd.h"
#include "apr_pools.h"
#include "apr_lib.h"
#include "apr_errno.h"
#include <stdlib.h>
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#include "ap_buckets.h"

API_EXPORT(apr_status_t) ap_bucket_destroy(ap_bucket *e)
{
    if (e->destroy) {
        e->destroy(e->data);
    }
    free(e);
    return APR_SUCCESS;
}

static apr_status_t ap_bucket_list_destroy(ap_bucket *e)
{
    ap_bucket *cur = e;
    ap_bucket *next;

    while (cur) {
        next = cur->next;
        ap_bucket_destroy(cur);
        cur = next;
    }
    return APR_SUCCESS;
}
static apr_status_t ap_brigade_cleanup(void *data)
{
    ap_bucket_brigade *b = data;

    ap_bucket_list_destroy(b->head);
    /* The brigade itself is allocated out of a pool, so we don't actually 
     * want to free it.  If we did, we would do that free() here.
     */
    return APR_SUCCESS;
}
API_EXPORT(apr_status_t) ap_brigade_destroy(ap_bucket_brigade *b)
{
    apr_kill_cleanup(b->p, b, ap_brigade_cleanup);
    return ap_brigade_cleanup(b);
}

API_EXPORT(ap_bucket_brigade *) ap_brigade_create(apr_pool_t *p)
{
    ap_bucket_brigade *b;

    b = apr_palloc(p, sizeof(*b));
    b->p = p;
    b->head = b->tail = NULL;

    apr_register_cleanup(b->p, b, ap_brigade_cleanup, ap_brigade_cleanup);
    return b;
}

API_EXPORT(void) ap_brigade_append_buckets(ap_bucket_brigade *b, 
                                                  ap_bucket *e)
{
    ap_bucket *cur = e;

    if (b->tail) {
        b->tail->next = e;
        e->prev = b->tail;
        while (cur->next) {
           cur = cur->next;
        }
        b->tail = cur;
    }
    else {
        b->head = b->tail = e;
    }
}

API_EXPORT(int) ap_brigade_to_iovec(ap_bucket_brigade *b, 
                                           struct iovec *vec, int nvec)
{
    ap_bucket *e;
    struct iovec *orig;

    orig = vec;
    e = b->head;
    while (e && nvec) {
	e->read(e, &vec->iov_base, &vec->iov_len, 0);
	e = e->next;
	--nvec;
	++vec;
    }
    return vec - orig;
}

API_EXPORT(void) ap_brigade_catenate(ap_bucket_brigade *a, 
                                            ap_bucket_brigade *b)
{
    if (b->head) {
        if (a->tail) {
            a->tail->next = b->head;
        }
        b->head->prev = a->tail;
	a->tail = b->tail;
        if (!a->head) {
            a->head = b->head;
        }
	b->head = NULL;
	b->tail = b->head;
    }
}

API_EXPORT(int) ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va)
{
    ap_bucket *r;
    const char *x;
    int j, k;
    apr_ssize_t i;

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

        ap_brigade_append_buckets(b, r);
    }

    return k;
}

API_EXPORT(int) ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...)
{
    va_list ap;
    int res;

    va_start(ap, fmt);
    res = ap_brigade_vprintf(b, fmt, ap);
    va_end(ap);
    return res;
}

API_EXPORT(int) ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va)
{
    /* XXX:  This needs to be replaced with a function to printf
     * directly into a bucket.  I'm being lazy right now.  RBB
     */
    char buf[4096];
    ap_bucket *r;
    int res, i;

    res = apr_vsnprintf(buf, 4096, fmt, va);

    r = ap_bucket_create_heap(buf, strlen(buf), 1, &i);
    ap_brigade_append_buckets(b, r);

    return res;
}
