/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#if APR_HAS_MMAP

static apr_status_t mmap_bucket_read(apr_bucket *b, const char **str, 
                                     apr_size_t *length, apr_read_type_e block)
{
    apr_bucket_mmap *m = b->data;
    apr_status_t ok;
    void *addr;
   
    if (!m->mmap) {
        /* the apr_mmap_t was already cleaned up out from under us */
        return APR_EINVAL;
    }

    ok = apr_mmap_offset(&addr, m->mmap, b->start);
    if (ok != APR_SUCCESS) {
        return ok;
    }
    *str = addr;
    *length = b->length;
    return APR_SUCCESS;
}

static apr_status_t mmap_bucket_cleanup(void *data)
{
    /* the apr_mmap_t is about to disappear out from under us, so we
     * have no choice but to pretend it doesn't exist anymore.  the
     * refcount is now useless because there's nothing to refer to
     * anymore.  so the only valid action on any remaining referrer
     * is to delete it.  no more reads, no more anything. */
    apr_bucket_mmap *m = data;

    m->mmap = NULL;
    return APR_SUCCESS;
}

static void mmap_bucket_destroy(void *data)
{
    apr_bucket_mmap *m = data;

    if (apr_bucket_shared_destroy(m)) {
        if (m->mmap) {
            apr_pool_cleanup_kill(m->mmap->cntxt, m, mmap_bucket_cleanup);
            apr_mmap_delete(m->mmap);
        }
        apr_bucket_free(m);
    }
}

/*
 * XXX: are the start and length arguments useful?
 */
APU_DECLARE(apr_bucket *) apr_bucket_mmap_make(apr_bucket *b, apr_mmap_t *mm, 
                                               apr_off_t start, 
                                               apr_size_t length)
{
    apr_bucket_mmap *m;

    m = apr_bucket_alloc(sizeof(*m), b->list);
    m->mmap = mm;

    apr_pool_cleanup_register(mm->cntxt, m, mmap_bucket_cleanup,
                              apr_pool_cleanup_null);

    b = apr_bucket_shared_make(b, m, start, length);
    b->type = &apr_bucket_type_mmap;

    return b;
}


APU_DECLARE(apr_bucket *) apr_bucket_mmap_create(apr_mmap_t *mm, 
                                                 apr_off_t start, 
                                                 apr_size_t length,
                                                 apr_bucket_alloc_t *list)
{
    apr_bucket *b = apr_bucket_alloc(sizeof(*b), list);

    APR_BUCKET_INIT(b);
    b->free = apr_bucket_free;
    b->list = list;
    return apr_bucket_mmap_make(b, mm, start, length);
}

static apr_status_t mmap_bucket_setaside(apr_bucket *b, apr_pool_t *p)
{
    apr_bucket_mmap *m = b->data;
    apr_mmap_t *mm = m->mmap;
    apr_mmap_t *new_mm;
    apr_status_t ok;

    if (!mm) {
        /* the apr_mmap_t was already cleaned up out from under us */
        return APR_EINVAL;
    }

    /* shortcut if possible */
    if (apr_pool_is_ancestor(mm->cntxt, p)) {
        return APR_SUCCESS;
    }

    /* duplicate apr_mmap_t into new pool */
    /* XXX: the transfer_ownership flag on this call
     * will go away soon.. it's ignored right now. */
    ok = apr_mmap_dup(&new_mm, mm, p, 1);
    if (ok != APR_SUCCESS) {
        return ok;
    }

    /* decrement refcount on old apr_bucket_mmap */
    mmap_bucket_destroy(m);

    /* create new apr_bucket_mmap pointing to new apr_mmap_t */
    apr_bucket_mmap_make(b, new_mm, b->start, b->length);

    return APR_SUCCESS;
}

APU_DECLARE_DATA const apr_bucket_type_t apr_bucket_type_mmap = {
    "MMAP", 5, APR_BUCKET_DATA,
    mmap_bucket_destroy,
    mmap_bucket_read,
    mmap_bucket_setaside,
    apr_bucket_shared_split,
    apr_bucket_shared_copy
};

#endif
