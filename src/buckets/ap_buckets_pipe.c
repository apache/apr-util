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
static apr_status_t pipe_read(ap_bucket *a, const char **str,
			      apr_ssize_t *len, int block)
{
    apr_file_t *p = a->data;
    ap_bucket *b;
    char *buf;
    apr_status_t rv;

    buf = malloc(IOBUFSIZE); /* XXX: check for failure? */
    *str = buf;
    *len = IOBUFSIZE;
    rv = apr_read(p, buf, len);
    if (rv != APR_SUCCESS && rv != APR_EOF) {
	free(buf);
        return rv;
    }
    /*
     * Change the current bucket to refer to what we read,
     * even if we read nothing because we hit EOF.
     */
    ap_bucket_make_heap(a, buf, *len, 0, NULL);  /* XXX: check for failure? */
    /*
     * If there's more to read we have to keep the rest of the pipe
     * for later. XXX: Note that more complicated bucket types that
     * refer to data not in memory and must therefore have a read()
     * function similar to this one should be wary of copying this
     * code because if they have a destroy function they probably
     * want to migrate the bucket's subordinate structure from the
     * old bucket to a raw new one and adjust it as appropriate,
     * rather than destroying the old one and creating a completely
     * new bucket.
     */
    if (*len > 0) {
        b = ap_bucket_create_pipe(p);
	AP_BUCKET_INSERT_AFTER(a, b);
    }
    return APR_SUCCESS;
}

API_EXPORT(ap_bucket *) ap_bucket_make_pipe(ap_bucket *b, apr_file_t *p)
{
    /*
     * XXX: We rely on a cleanup on some pool or other to actually
     * destroy the pipe. We should probably explicitly call apr to
     * destroy it instead.
     *
     * Note that typically the pipe is allocated from the request pool
     * so it will disappear when the request is finished. However the
     * core filter may decide to set aside the tail end of a CGI
     * response if the connection is pipelined. This turns out not to
     * be a problem because the core will have read to the end of the
     * stream so the bucket(s) that it sets aside will be the heap
     * buckets created by pipe_read() above.
     */
    b->type     = &ap_pipe_type;
    b->length   = -1;
    b->data     = p;

    return b;
}

API_EXPORT(ap_bucket *) ap_bucket_create_pipe(apr_file_t *p)
{
    ap_bucket_do_create(ap_bucket_make_pipe(b, p));
}

const ap_bucket_type ap_pipe_type = {
    "PIPE", 4,
    ap_bucket_destroy_notimpl,
    pipe_read,
    ap_bucket_setaside_notimpl,
    ap_bucket_split_notimpl
};
