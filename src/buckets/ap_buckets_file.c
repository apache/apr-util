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

/* XXX: We should obey the block flag */
static apr_status_t file_read(ap_bucket *a, const char **str,
			      apr_ssize_t *len, int block)
{
    apr_file_t *f = (apr_file_t *) a->data;
    ap_bucket *b = NULL;
    char *buf;
    apr_status_t rv;

    buf = malloc(IOBUFSIZE);
    *str = buf;

    if (a->length > IOBUFSIZE) {
        *len = IOBUFSIZE;
    }
    else {
        *len = a->length;
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
    ap_bucket_make_heap(a, buf, *len, 0, NULL);  /* XXX: check for failure? */

    /* If we have more to read from the file, then create another bucket */
    if (*len > 0) {
        b = ap_bucket_create_file(f, 0, a->length);
	AP_BUCKET_INSERT_AFTER(a, b);
    }
    return APR_SUCCESS;
}

API_EXPORT(ap_bucket *) ap_bucket_make_file(ap_bucket *b, apr_file_t *fd,
                                            apr_off_t offset, apr_size_t len)
{
    b->type = AP_BUCKET_FILE;
    b->data = fd;
    b->length = len;
    b->offset = offset;
    b->destroy = NULL;
    b->read = file_read;
    b->setaside = NULL;
    b->split = NULL;

    return b;
}

API_EXPORT(ap_bucket *) ap_bucket_create_file(apr_file_t *fd, apr_off_t offset, apr_size_t len)
{
    ap_bucket_do_create(ap_bucket_make_file(b, fd, offset, len));
}
