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

static int heap_type;
/*
 * The size of heap bucket memory allocations.
 * XXX: This is currently a guess and should be adjusted to an
 * empirically good value.
 */
#ifndef DEFAULT_BUCKET_SIZE
#define DEFAULT_BUCKET_SIZE (4096)
#endif

static apr_status_t heap_read(ap_bucket *b, const char **str, 
			      apr_ssize_t *len, int block)
{
    ap_bucket_shared *s = b->data;
    ap_bucket_heap *h = s->data;

    *str = h->base + s->start;
    *len = s->end - s->start;
    return APR_SUCCESS;
}

static void heap_destroy(void *data)
{
    ap_bucket_heap *h;

    h = ap_bucket_destroy_shared(data);
    if (h == NULL) {
	return;
    }
    free(h->base);
    free(h);
}

API_EXPORT(ap_bucket *) ap_bucket_make_heap(ap_bucket *b,
		const char *buf, apr_size_t length, int copy, apr_ssize_t *w)
{
    ap_bucket_heap *h;

    h = malloc(sizeof(*h));
    if (h == NULL) {
	return NULL;
    }

    if (copy) {
	h->base = malloc(DEFAULT_BUCKET_SIZE);
	if (h->base == NULL) {
	    free(h);
	    return NULL;
	}
	h->alloc_len = DEFAULT_BUCKET_SIZE;
	if (length > DEFAULT_BUCKET_SIZE) {
	    length = DEFAULT_BUCKET_SIZE;
	}
	memcpy(h->base, buf, length);
    }
    else {
	/* XXX: we lose the const qualifier here which indicates
         * there's something screwy with the API...
	 */
	h->base = (char *) buf;
	h->alloc_len = length;
    }

    b = ap_bucket_make_shared(b, h, 0, length);
    if (b == NULL) {
	if (copy) {
	    free(h->base);
	}
	free(h);
	return NULL;
    }

    b->type     = heap_type;

    if (w)
        *w = length;

    return b;
}

API_EXPORT(ap_bucket *) ap_bucket_create_heap(
		const char *buf, apr_size_t length, int copy, apr_ssize_t *w)
{
    ap_bucket_do_create(ap_bucket_make_heap(b, buf, length, copy, w));
}

void ap_bucket_heap_register(apr_pool_t *p)
{
    ap_bucket_type type;

    type.split    = ap_bucket_split_shared;
    type.destroy  = heap_destroy;
    type.read     = heap_read;
    type.setaside = NULL;

    heap_type = ap_insert_bucket_type(&type);
}

int ap_heap_type(void)
{
    return heap_type;
}
