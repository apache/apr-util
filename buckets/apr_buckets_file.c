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

#include "apr_lib.h"
#include "apr_file_io.h"
#include "ap_buckets.h"
#include <stdlib.h>

/* Allow Apache to use ap_mmap */
#ifdef AP_USE_MMAP_FILES
#include "apr_mmap.h"

/* mmap support for static files based on ideas from John Heidemann's
 * patch against 1.0.5.  See
 * <http://www.isi.edu/~johnh/SOFTWARE/APACHE/index.html>.
 */

/* Files have to be at least this big before they're mmap()d.  This is to deal
 * with systems where the expense of doing an mmap() and an munmap() outweighs
 * the benefit for small files.  It shouldn't be set lower than 1.
 */
#ifndef MMAP_THRESHOLD
#  ifdef SUNOS4
#  define MMAP_THRESHOLD                (8*1024)
#  else
#  define MMAP_THRESHOLD                1
#  endif /* SUNOS4 */
#endif /* MMAP_THRESHOLD */
#ifndef MMAP_LIMIT
#define MMAP_LIMIT              (4*1024*1024)
#endif
#endif /* AP_USE_MMAP_FILES */


/* XXX: We should obey the block flag */
static apr_status_t file_read(ap_bucket *e, const char **str,
			      apr_size_t *len, ap_read_type block)
{
    ap_bucket_file *a = (ap_bucket_file *)e->data;
    apr_file_t *f = (apr_file_t *) a->fd;
    ap_bucket *b = NULL;
    char *buf;
    apr_status_t rv;
#ifdef AP_USE_MMAP_FILES
    apr_mmap_t *mm = NULL;
#endif

#ifdef AP_USE_MMAP_FILES
    if ((e->length >= MMAP_THRESHOLD)
        && (e->length < MMAP_LIMIT)) {
        /* we need to protect ourselves in case we die while we've got the
         * file mmapped */
        apr_status_t status;
        if ((status = apr_mmap_create(&mm, f, a->offset, e->length, 
                                      APR_MMAP_READ, NULL)) != APR_SUCCESS) {
            mm = NULL;
        }
    }
    else {
        mm = NULL;
    }
    if (mm) {
        ap_bucket_make_mmap(e, mm, 0, e->length); /*XXX: check for failure? */
        return ap_bucket_read(e, str, len, block);
    }
    else {
#endif

        buf = malloc(HUGE_STRING_LEN);
        *str = buf;

        if (e->length > HUGE_STRING_LEN) {
            *len = HUGE_STRING_LEN;
        }
        else {
            *len = e->length;
        }

        /* Handle offset ... */
        if (a->offset) {
            rv = apr_seek(f, APR_SET, &a->offset);
            if (rv != APR_SUCCESS) {
                free(buf);
                return rv;
            }
            /* Only need to do seek the first time through */
            a->offset = 0;
        }
        rv = apr_read(f, buf, len);
        if (rv != APR_SUCCESS && rv != APR_EOF) {
	    free(buf);
            return rv;
        }

        /*
         * Change the current bucket to refer to what we read,
         * even if we read nothing because we hit EOF.
         */
        ap_bucket_make_heap(e, buf, *len, 0, NULL); /*XXX: check for failure? */

        /* If we have more to read from the file, then create another bucket */
        if (*len > 0) {
            b = ap_bucket_create_file(f, 0, e->length);
            AP_BUCKET_INSERT_AFTER(e, b);
        }
#ifdef AP_USE_MMAP_FILES
    }
#endif
    return APR_SUCCESS;
}

APR_DECLARE(ap_bucket *) ap_bucket_make_file(ap_bucket *b, apr_file_t *fd,
                                            apr_off_t offset, apr_size_t len)
{
    ap_bucket_file *f;

    f = malloc(sizeof(*f));
    if (f == NULL) {
        return NULL;
    }
 
    f->fd = fd;
    f->offset = offset;

    b->type = &ap_file_type;
    b->data = f;
    b->length = len;

    return b;
}

APR_DECLARE(ap_bucket *) ap_bucket_create_file(apr_file_t *fd,
                                              apr_off_t offset, apr_size_t len)
{
    ap_bucket_do_create(ap_bucket_make_file(b, fd, offset, len));
}

APR_DECLARE_DATA const ap_bucket_type ap_file_type = {
    "FILE", 4,
    ap_bucket_destroy_notimpl,
    file_read,
    ap_bucket_setaside_notimpl,
    ap_bucket_split_notimpl
};
