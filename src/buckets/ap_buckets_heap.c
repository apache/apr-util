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

#ifndef DEFAULT_RWBUF_SIZE
#define DEFAULT_RWBUF_SIZE (4096)
#endif

void ap_heap_setaside(ap_bucket *e);

static apr_status_t heap_get_str(ap_bucket *e, const char **str, 
                                 apr_ssize_t *len, int block)
{
    ap_bucket_heap *b = (ap_bucket_heap *)e->data;
    *str = b->start;
    *len = e->length;
    return APR_SUCCESS;
}

static void heap_destroy(ap_bucket *e)
{
    ap_bucket_heap *d = (ap_bucket_heap *)e->data;
    free(d->alloc_addr);
}

static apr_status_t heap_split(ap_bucket *e, apr_off_t nbyte)
{
    ap_bucket *newbuck;
    ap_bucket_heap *a = (ap_bucket_heap *)e;
    ap_bucket_heap *b;
    apr_ssize_t dump; 

    newbuck = ap_bucket_heap_create(a->alloc_addr, a->alloc_len, &dump);
    b = (ap_bucket_heap *)newbuck->data;

    b->alloc_addr = a->alloc_addr;
    b->alloc_len = a->alloc_len;
    b->end = a->end;
    a->end = (char *) a->start + nbyte;
    b->start = (char *) a->end + 1;
    newbuck->length = e->length - nbyte;
    e->length = nbyte;

    newbuck->prev = e;
    newbuck->next = e->next;
    e->next = newbuck;

    return APR_SUCCESS;
}

/*
 * save nbyte bytes to the bucket.
 * Only returns fewer than nbyte if an error occurred.
 * Returns -1 if no bytes were written before the error occurred.
 * It is worth noting that if an error occurs, the buffer is in an unknown
 * state.
 */
static apr_status_t heap_insert(ap_bucket *e, const void *buf,
                                apr_size_t nbyte, apr_ssize_t *w)
{
    int amt;
    int total;
    ap_bucket_heap *b = (ap_bucket_heap *)e->data;

    if (nbyte == 0) {
        *w = 0;
        return APR_SUCCESS;
    }

/*
 * At this point, we need to make sure we aren't trying to write too much
 * data to the bucket.  We will need to write to the dist here, but I am
 * leaving that for a later pass.  The basics are presented below, but this
 * is horribly broken.
 */
    amt = b->alloc_len - ((char *)b->end - (char *)b->start);
    total = 0;
    if (nbyte > amt) {
        /* loop through and write to the disk */
        /* Replace the heap buckets with file buckets */
    }
    /* now we know that nbyte < b->alloc_len */
    memcpy(b->end, buf, nbyte);
    b->end = (char *)b->end + nbyte;
    *w = total + nbyte;
    return APR_SUCCESS;
}

void ap_heap_setaside(ap_bucket *e)
{
    ap_bucket_transient *a = (ap_bucket_transient *)e;
    ap_bucket_heap *b = calloc(1, sizeof(*b));

    b->alloc_addr = calloc(DEFAULT_RWBUF_SIZE, 1);
    b->alloc_len  = DEFAULT_RWBUF_SIZE;
    memcpy(b->alloc_addr, a->start, e->length);
    b->start      = b->alloc_addr;
    b->end        = (char *) b->start + e->length;

    e->type       = AP_BUCKET_HEAP;
    e->read       = heap_get_str;
    e->setaside   = NULL;
    e->split      = heap_split;
    e->destroy    = heap_destroy;
}

API_EXPORT(ap_bucket *) ap_bucket_heap_create(const void *buf,
                                apr_size_t nbyte, apr_ssize_t *w)
{
    ap_bucket *newbuf;
    ap_bucket_heap *b;

    newbuf = calloc(1, sizeof(*newbuf));
    b = malloc(sizeof(*b));
    
    b->alloc_addr = calloc(DEFAULT_RWBUF_SIZE, 1);
    b->alloc_len  = DEFAULT_RWBUF_SIZE;
    b->start      = b->alloc_addr;
    b->end        = b->alloc_addr;

    newbuf->data       = b;
    heap_insert(newbuf, buf, nbyte, w); 
    newbuf->length     = (char *) b->end - (char *) b->start;

    newbuf->type       = AP_BUCKET_HEAP;
    newbuf->read       = heap_get_str;
    newbuf->setaside   = NULL;
    newbuf->split      = heap_split;
    newbuf->destroy    = heap_destroy;

    return newbuf;
}

