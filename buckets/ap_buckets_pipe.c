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

#include "apr_private.h"
#include "httpd.h"
#include "ap_buckets.h"
#include <stdlib.h>

static apr_status_t pipe_split(ap_bucket *a, apr_off_t point)
{
    /* Splitting a pipe doesn't really make any sense, because as we read
     * it becomes a heap bucket.  I am leaving this in because I may be wrong,
     * but it just returns an error, and I expect it to go away immediately
     */
    return APR_ENOTIMPL;
}

/* Ignore the block arg for now.  We can fix that tomorrow. */
static apr_status_t pipe_read(ap_bucket *b, const char **str, 
				apr_ssize_t *len, int block)
{
    ap_bucket_pipe *bd = b->data;
    ap_bucket *a;
    apr_size_t l;
    apr_ssize_t toss;
    char buf[IOBUFSIZE];
    apr_status_t rv;

    *len = IOBUFSIZE;
    if ((rv = apr_read(bd->thepipe, buf, len)) != APR_SUCCESS) {
        return rv;
    }
    if (*len > 0) {
        l = *len;
        a = ap_bucket_create_pipe(bd->thepipe);
        
        /* XXX ap_bucket_make_heap() can decide not to copy all our data;
         * either handle it here or ensure that IOBUFSIZE < 
         * DEFAULT_BUCKET_SIZE;
         */
        b = ap_bucket_make_heap(b, buf, l, 1, &toss);
        b->read(b, str, len, block); /* set str to new location of data */

        b->next->prev = a;
        a->next = b->next;
        b->next = a;
        a->prev = b;
    }
    return APR_SUCCESS;
}

API_EXPORT(ap_bucket *) ap_bucket_make_pipe(ap_bucket *b, apr_file_t *thispipe)
{
    ap_bucket_pipe *bd;

    bd = malloc(sizeof(*bd));
    if (bd == NULL) {
	return NULL;
    }
    bd->thepipe = thispipe;

    b->type     = AP_BUCKET_PIPE;
    b->length   = -1;
    b->setaside = NULL;
    b->destroy  = free;
    b->split    = NULL;
    b->read     = pipe_read;
    b->data     = bd;

    return b;
}

API_EXPORT(ap_bucket *) ap_bucket_create_pipe(apr_file_t *thispipe)
{
    ap_bucket_do_create(ap_bucket_make_pipe(b, thispipe));
}
