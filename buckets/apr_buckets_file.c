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

#include "apr.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_file_io.h"
#include "apr_buckets.h"
#include <stdlib.h>

/* Allow Apache to use ap_mmap */
#if APR_HAS_MMAP
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
#endif /* APR_HAS_MMAP */


/* XXX: We should obey the block flag */
static apr_status_t file_read(apr_bucket *e, const char **str,
			      apr_size_t *len, apr_read_type_e block)
{
    apr_bucket_file *a = (apr_bucket_file *)e->data;
    apr_file_t *f = (apr_file_t *) a->fd;
    apr_bucket *b = NULL;
    char *buf;
    apr_status_t rv;
    apr_off_t length = e->length;
#if APR_HAS_MMAP
    apr_mmap_t *mm = NULL;
#endif

#if APR_HAS_MMAP
    if ((e->length >= MMAP_THRESHOLD)
        && (e->length < MMAP_LIMIT)) {
        /* we need to protect ourselves in case we die while we've got the
         * file mmapped */
        apr_status_t status;
        apr_pool_t *p = APR_GET_POOL(f);
        if ((status = apr_mmap_create(&mm, f, a->offset, e->length, 
                                      APR_MMAP_READ, p)) != APR_SUCCESS) {
            mm = NULL;
        }
    }
    else {
        mm = NULL;
    }
    if (mm) {
        apr_bucket_make_mmap(e, mm, 0, e->length); /*XXX: check for failure? */
        return apr_bucket_read(e, str, len, block);
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
        }
        rv = apr_read(f, buf, len);
        if (rv != APR_SUCCESS && rv != APR_EOF) {
	    free(buf);
            return rv;
        }
        length -= *len;

        /*
         * Change the current bucket to refer to what we read,
         * even if we read nothing because we hit EOF.
         */
        apr_bucket_make_heap(e, buf, *len, 0, NULL); /*XXX: check for failure? */

        /* If we have more to read from the file, then create another bucket */
        if (length > 0) {
            b = apr_bucket_create_file(f, a->offset + (*len), length);
            APR_BUCKET_INSERT_AFTER(e, b);
        }
#if APR_HAS_MMAP
    }
#endif
    return APR_SUCCESS;
}

APU_DECLARE(apr_bucket *) apr_bucket_make_file(apr_bucket *b, apr_file_t *fd,
                                            apr_off_t offset, apr_size_t len)
{
    apr_bucket_file *f;

    f = malloc(sizeof(*f));
    if (f == NULL) {
        return NULL;
    }
 
    f->fd = fd;
    f->offset = offset;

    b->type = &apr_bucket_type_file;
    b->data = f;
    b->length = len;

    return b;
}

APU_DECLARE(apr_bucket *) apr_bucket_create_file(apr_file_t *fd,
                                              apr_off_t offset, apr_size_t len)
{
    apr_bucket_do_create(apr_bucket_make_file(b, fd, offset, len));
}

APU_DECLARE_DATA const apr_bucket_type_t apr_bucket_type_file = {
    "FILE", 5,
    free,
    file_read,
    apr_bucket_setaside_notimpl,
    apr_bucket_split_notimpl,
    apr_bucket_copy_notimpl
};
