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

#include "ap_buckets.h"
#include <stdlib.h>

static apr_status_t mmap_read(ap_bucket *b, const char **str, 
			      apr_size_t *length, ap_read_type block)
{
    ap_bucket_shared *s = b->data;
    ap_bucket_mmap *m = s->data;
    apr_status_t ok;
    void *addr;
    
    ok = apr_mmap_offset(&addr, m->mmap, s->start);
    if (ok != APR_SUCCESS) {
	return ok;
    }
    *str = addr;
    *length = s->end - s->start;
    return APR_SUCCESS;
}

static void mmap_destroy(void *data)
{
    ap_bucket_mmap *m;

    m = ap_bucket_destroy_shared(data);
    if (m == NULL) {
	return;
    }
    free(m);
}

/*
 * XXX: are the start and length arguments useful?
 */
APR_DECLARE(ap_bucket *) ap_bucket_make_mmap(ap_bucket *b,
		apr_mmap_t *mm, apr_off_t start, apr_size_t length)
{
    ap_bucket_mmap *m;

    m = malloc(sizeof(*m));
    if (m == NULL) {
	return NULL;
    }
    m->mmap = mm;

    b = ap_bucket_make_shared(b, m, start, start+length);
    if (b == NULL) {
	free(m);
	return NULL;
    }

    b->type     = &ap_mmap_type;

    return b;
}


APR_DECLARE(ap_bucket *) ap_bucket_create_mmap(
		apr_mmap_t *mm, apr_off_t start, apr_size_t length)
{
    ap_bucket_do_create(ap_bucket_make_mmap(b, mm, start, length));
}

APR_DECLARE_DATA const ap_bucket_type ap_mmap_type = {
    "MMAP", 5,
    mmap_destroy,
    mmap_read,
    ap_bucket_setaside_notimpl,
    ap_bucket_split_shared,
    ap_bucket_copy_shared
};
