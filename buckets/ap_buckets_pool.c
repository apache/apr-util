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
#include "ap_buckets.h"
#include <stdlib.h>

static apr_status_t pool_bucket_cleanup(void *data)
{
    ap_bucket_shared *s = data;
    ap_bucket_shared *new;
    ap_bucket_pool *h = s->data;
    ap_bucket *b = h->b;
    apr_ssize_t w;

    ap_bucket_make_heap(b, h->base, b->length, 1, &w);
    new = b->data;

    new->start = s->start;
    new->end = s->end;

    ap_bucket_destroy_shared(s);
    return APR_SUCCESS;
}

static apr_status_t pool_read(ap_bucket *b, const char **str, 
			      apr_ssize_t *len, int block)
{
    ap_bucket_shared *s = b->data;
    ap_bucket_pool *h = s->data;

    *str = h->base + s->start;
    *len = s->end - s->start;
    return APR_SUCCESS;
}

static void pool_destroy(void *data)
{
    ap_bucket_shared *s = data;
    ap_bucket_pool *h = s->data;

    apr_kill_cleanup(h->p, data, pool_bucket_cleanup);
    h = ap_bucket_destroy_shared(data);
    if (h == NULL) {
	return;
    }
    free(h);
}

AP_DECLARE(ap_bucket *) ap_bucket_make_pool(ap_bucket *b,
		const char *buf, apr_size_t length, apr_pool_t *p)
{
    ap_bucket_pool *h;

    h = malloc(sizeof(*h));
    if (h == NULL) {
	return NULL;
    }

    /* XXX: we lose the const qualifier here which indicates
     * there's something screwy with the API...
     */
    h->base = (char *) buf;
    h->p    = p;

    b = ap_bucket_make_shared(b, h, 0, length);
    if (b == NULL) {
	free(h);
	return NULL;
    }

    b->type = &ap_pool_type;

    apr_register_cleanup(h->p, b->data, pool_bucket_cleanup, apr_null_cleanup);
    return b;
}

AP_DECLARE(ap_bucket *) ap_bucket_create_pool(
		const char *buf, apr_size_t length, apr_pool_t *p)
{
    ap_bucket_do_create(ap_bucket_make_pool(b, buf, length, p));
}

const ap_bucket_type ap_pool_type = {
    "POOL", 4,
    pool_destroy,
    pool_read,
    ap_bucket_setaside_notimpl,
    ap_bucket_split_shared
};
