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

#include <stdlib.h>

#include "apr_errno.h"

#include "ap_buckets.h"

APU_DECLARE_NONSTD(apr_status_t) ap_bucket_split_shared(ap_bucket *a, apr_off_t point)
{
    ap_bucket *b;
    ap_bucket_shared *ad, *bd;
    apr_status_t rv;

    if (point < 0 || point > a->length) {
	return APR_EINVAL;
    }

    rv = ap_bucket_copy_shared(a, &b);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    ad = a->data;
    bd = b->data;

    a->length = point;
    ad->end = ad->start + point;
    b->length -= point;
    bd->start += point;

    AP_BUCKET_INSERT_AFTER(a, b);

    return APR_SUCCESS;
}

APU_DECLARE_NONSTD(apr_status_t) ap_bucket_copy_shared(ap_bucket *a, ap_bucket **c)
{
    ap_bucket *b;
    ap_bucket_shared *ad, *bd;
    ap_bucket_refcount *r;

    b = malloc(sizeof(*b));
    if (b == NULL) {
        return APR_ENOMEM;
    }
    bd = malloc(sizeof(*bd));
    if (bd == NULL) {
        free(b);
        return APR_ENOMEM;
    }
    *b = *a;
    ad = a->data;
    b->data = bd;
    *bd = *ad;

    r = ad->data;
    r->refcount += 1;

    *c = b;

    return APR_SUCCESS;
}

APU_DECLARE(void *) ap_bucket_destroy_shared(void *data)
{
    ap_bucket_shared *s = data;
    ap_bucket_refcount *r = s->data;

    free(s);
    r->refcount -= 1;
    if (r->refcount == 0) {
	return r;
    }
    else {
	return NULL;
    }
}

APU_DECLARE(ap_bucket *) ap_bucket_make_shared(ap_bucket *b, void *data,
					      apr_off_t start, apr_off_t end)
{
    ap_bucket_shared *s;
    ap_bucket_refcount *r = data;

    s = malloc(sizeof(*s));
    if (s == NULL) {
	return NULL;
    }

    b->data = s;
    b->length = end - start;
    /* caller initializes the type field and function pointers */
    s->start = start;
    s->end = end;
    s->data = r;
    r->refcount = 1;
    /* caller initializes the rest of r */

    return b;
}
