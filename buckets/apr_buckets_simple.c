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

#include "apr_buckets.h"
#include <stdlib.h>

/*
 * We can't simplify this function by using an apr_bucket_make function
 * because we aren't sure of the exact type of this bucket.
 */
static apr_status_t simple_copy(apr_bucket *a, apr_bucket **c)
{
    apr_bucket *b;
    apr_bucket_simple *ad, *bd;

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

    *c = b;

    return APR_SUCCESS;
}

static apr_status_t simple_split(apr_bucket *a, apr_off_t point)
{
    apr_bucket *b;
    apr_bucket_simple *ad, *bd;
    apr_status_t rv;

    if (point < 0 || point > a->length) {
	return APR_EINVAL;
    }

    rv = simple_copy(a, &b);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    ad = a->data;
    bd = b->data;

    a->length = point;
    ad->end = ad->start + point;
    b->length -= point;
    bd->start += point;

    APR_BUCKET_INSERT_AFTER(a, b);

    return APR_SUCCESS;
}

static apr_status_t simple_read(apr_bucket *b, const char **str, 
				apr_size_t *len, apr_read_type_e block)
{
    apr_bucket_simple *bd = b->data;
    *str = bd->start;
    *len = bd->end - bd->start;
    return APR_SUCCESS;
}

APU_DECLARE(apr_bucket *) apr_bucket_immortal_make(apr_bucket *b,
		const char *buf, apr_size_t length)
{
    apr_bucket_simple *bd;

    bd = malloc(sizeof(*bd));
    if (bd == NULL) {
	return NULL;
    }

    bd->start   = buf;
    bd->end     = buf+length;

    b->type     = &apr_bucket_type_immortal;
    b->length   = length;
    b->data     = bd;

    return b;
}

APU_DECLARE(apr_bucket *) apr_bucket_immortal_create(
		const char *buf, apr_size_t length)
{
    apr_bucket_do_create(apr_bucket_immortal_make(b, buf, length));
}

/*
 * XXX: This function could do with some tweaking to reduce memory
 * usage in various cases, e.g. share buffers in the heap between all
 * the buckets that are set aside, or even spool set-aside data to
 * disk if it gets too voluminous (but if it does then that's probably
 * a bug elsewhere). There should probably be a apr_brigade_setaside()
 * function that co-ordinates the action of all the bucket setaside
 * functions to improve memory efficiency.
 */
static apr_status_t transient_setaside(apr_bucket *b)
{
    apr_bucket_simple *bd;
    const char *start, *end;
    apr_size_t w;
    
    bd = b->data;
    start = bd->start;
    end = bd->end;
    /* XXX: handle small heap buckets */
    b = apr_bucket_heap_make(b, start, end-start, 1, &w);
    if (b == NULL || w != end-start) {
	return APR_ENOMEM;
    }
    free(bd);
    return APR_SUCCESS;
}

APU_DECLARE(apr_bucket *) apr_bucket_transient_make(apr_bucket *b,
		const char *buf, apr_size_t length)
{
    b = apr_bucket_immortal_make(b, buf, length);
    if (b == NULL) {
	return NULL;
    }
    b->type = &apr_bucket_type_transient;
    return b;
}

APU_DECLARE(apr_bucket *) apr_bucket_transient_create(
		const char *buf, apr_size_t length)
{
    apr_bucket_do_create(apr_bucket_transient_make(b, buf, length));
}

const apr_bucket_type_t apr_bucket_type_immortal = {
    "IMMORTAL", 5,
    free,
    simple_read,
    apr_bucket_notimpl_setaside,
    simple_split,
    simple_copy
};

APU_DECLARE_DATA const apr_bucket_type_t apr_bucket_type_transient = {
    "TRANSIENT", 5,
    free, 
    simple_read,
    transient_setaside,
    simple_split,
    simple_copy
};
