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
#include "apr_buckets.h"
#include <stdlib.h>

static apr_status_t pipe_read(apr_bucket *a, const char **str,
			      apr_size_t *len, apr_read_type_e block)
{
    apr_file_t *p = a->data;
    apr_bucket *b;
    char *buf;
    apr_status_t rv;
    apr_interval_time_t timeout;

    if (block == APR_NONBLOCK_READ) {
        apr_file_pipe_timeout_get(p, &timeout);
        apr_file_pipe_timeout_set(p, 0);
    }

    buf = malloc(HUGE_STRING_LEN); /* XXX: check for failure? */
    *str = buf;
    *len = HUGE_STRING_LEN;
    rv = apr_file_read(p, buf, len);

    if (block == APR_NONBLOCK_READ) {
        apr_file_pipe_timeout_set(p, timeout);
    }

    if (rv != APR_SUCCESS && rv != APR_EOF) {
        *str = NULL;
	free(buf);
        return rv;
    }
    /*
     * Change the current bucket to refer to what we read,
     * even if we read nothing because we hit EOF.
     */
    apr_bucket_heap_make(a, buf, *len, 0, NULL);  /* XXX: check for failure? */
    /*
     * If there's more to read we have to keep the rest of the pipe
     * for later.  Otherwise, we'll close the pipe.
     * XXX: Note that more complicated bucket types that 
     * refer to data not in memory and must therefore have a read()
     * function similar to this one should be wary of copying this
     * code because if they have a destroy function they probably
     * want to migrate the bucket's subordinate structure from the
     * old bucket to a raw new one and adjust it as appropriate,
     * rather than destroying the old one and creating a completely
     * new bucket.
     */
    if (*len > 0) {
        b = apr_bucket_pipe_creat(p);
	APR_BUCKET_INSERT_AFTER(a, b);
    }
    else if (rv == APR_EOF) {
        apr_file_close(p);
        return (block == APR_NONBLOCK_READ) ? APR_EOF : APR_SUCCESS;
    }
    return APR_SUCCESS;
}

APU_DECLARE(apr_bucket *) apr_bucket_pipe_make(apr_bucket *b, apr_file_t *p)
{
    /*
     * A pipe is closed when the end is reached in pipe_read().  If the
     * pipe isn't read to the end (e.g., error path), the pipe will be
     * closed when its pool goes away.
     *
     * Note that typically the pipe is allocated from the request pool
     * so it will disappear when the request is finished. However the
     * core filter may decide to set aside the tail end of a CGI
     * response if the connection is pipelined. This turns out not to
     * be a problem because the core will have read to the end of the
     * stream so the bucket(s) that it sets aside will be the heap
     * buckets created by pipe_read() above.
     */
    b->type     = &apr_bucket_type_pipe;
    b->length   = -1;
    b->data     = p;

    return b;
}

APU_DECLARE(apr_bucket *) apr_bucket_pipe_creat(apr_file_t *p)
{
    apr_bucket_do_create(apr_bucket_pipe_make(b, p));
}

APU_DECLARE_DATA const apr_bucket_type_t apr_bucket_type_pipe = {
    "PIPE", 5,
    apr_bucket_notimpl_destroy,
    pipe_read,
    apr_bucket_notimpl_setaside,
    apr_bucket_notimpl_split,
    apr_bucket_notimpl_copy
};
