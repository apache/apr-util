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

#include "apr_buckets.h"
#include <stdlib.h>
#if APR_HAVE_STRINGS_H
#include <strings.h>
#endif

static apr_status_t heap_read(apr_bucket *b, const char **str, 
			      apr_size_t *len, apr_read_type_e block)
{
    apr_bucket_shared *s = b->data;
    apr_bucket_heap *h = s->data;

    *str = h->base + s->start;
    *len = s->end - s->start;
    return APR_SUCCESS;
}

static void heap_destroy(void *data)
{
    apr_bucket_heap *h;

    h = apr_bucket_shared_destroy(data);
    if (h == NULL) {
	return;
    }
    free(h->base);
    free(h);
}

APU_DECLARE(apr_bucket *) apr_bucket_heap_make(apr_bucket *b,
		const char *buf, apr_size_t length, int copy, apr_size_t *w)
{
    apr_bucket_heap *h;

    h = malloc(sizeof(*h));
    if (h == NULL) {
	return NULL;
    }

    if (copy) {
	h->alloc_len = length;
	h->base = malloc(h->alloc_len);
	if (h->base == NULL) {
	    free(h);
	    return NULL;
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

    b = apr_bucket_shared_make(b, h, 0, length);
    if (b == NULL) {
	if (copy) {
	    free(h->base);
	}
	free(h);
	return NULL;
    }

    b->type = &apr_bucket_type_heap;

    if (w)
        *w = length;

    return b;
}

APU_DECLARE(apr_bucket *) apr_bucket_heap_create(
		const char *buf, apr_size_t length, int copy, apr_size_t *w)
{
    apr_bucket_do_create(apr_bucket_heap_make(b, buf, length, copy, w));
}

APU_DECLARE_DATA const apr_bucket_type_t apr_bucket_type_heap = {
    "HEAP", 5,
    heap_destroy,
    heap_read,
    apr_bucket_setaside_notimpl,
    apr_bucket_shared_split,
    apr_bucket_shared_copy
};
