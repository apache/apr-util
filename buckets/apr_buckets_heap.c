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
#define APR_WANT_MEMFUNC
#define APR_WANT_STRFUNC
#include "apr_want.h"
#include <stdlib.h>

static apr_status_t heap_read(apr_bucket *b, const char **str, 
			      apr_size_t *len, apr_read_type_e block)
{
    apr_bucket_heap *h = b->data;

    *str = h->base + b->start;
    *len = b->length;
    return APR_SUCCESS;
}

static void heap_destroy(void *data)
{
    apr_bucket_heap *h = data;

    if (apr_bucket_shared_destroy(h)) {
        free(h->base);
        apr_sms_free(h->sms, h);
    }
}

APU_DECLARE(apr_bucket *) apr_bucket_heap_make(apr_bucket *b,
		const char *buf, apr_size_t length, int copy, apr_size_t *w)
{
    apr_bucket_heap *h;

    h = (apr_bucket_heap *)apr_sms_malloc(b->sms, sizeof(*h));

    if (copy) {
	h->alloc_len = length;
	h->base = malloc(h->alloc_len);
	memcpy(h->base, buf, length);
    }
    else {
	/* XXX: we lose the const qualifier here which indicates
         * there's something screwy with the API...
	 */
	h->base = (char *) buf;
	h->alloc_len = length;
    }
    h->sms = b->sms;

    apr_bucket_shared_make(b, h, 0, length);
    b->type = &apr_bucket_type_heap;

    /* XXX: the w parm is useless and should go away */
    if (w)
        *w = length;

    return b;
}

APU_DECLARE(apr_bucket *) apr_bucket_heap_create(
		const char *buf, apr_size_t length, int copy, apr_size_t *w)
{
    apr_sms_t *sms;
    apr_bucket *b;

    if (!apr_bucket_global_sms) {
        apr_sms_std_create(&apr_bucket_global_sms);
    }
    sms = apr_bucket_global_sms;
    b = (apr_bucket *)apr_sms_malloc(sms, sizeof(*b));
    APR_BUCKET_INIT(b);
    b->sms = sms;
    return apr_bucket_heap_make(b, buf, length, copy, w);
}

APU_DECLARE_DATA const apr_bucket_type_t apr_bucket_type_heap = {
    "HEAP", 5,
    heap_destroy,
    heap_read,
    apr_bucket_setaside_noop,
    apr_bucket_shared_split,
    apr_bucket_shared_copy
};
