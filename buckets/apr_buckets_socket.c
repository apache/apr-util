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
#include "ap_buckets.h"
#include <stdlib.h>

/* XXX: We should obey the block flag */
static apr_status_t socket_read(ap_bucket *a, const char **str,
			      apr_size_t *len, ap_read_type block)
{
    apr_socket_t *p = a->data;
    ap_bucket *b;
    char *buf;
    apr_status_t rv;
    apr_int32_t timeout;

    if (block == AP_NONBLOCK_READ) {
        apr_getsocketopt(p, APR_SO_TIMEOUT, &timeout);
        apr_setsocketopt(p, APR_SO_TIMEOUT, 0);
    }

    buf = malloc(HUGE_STRING_LEN); /* XXX: check for failure? */
    *str = buf;
    *len = HUGE_STRING_LEN;
    rv = apr_recv(p, buf, len);

    if (block == AP_NONBLOCK_READ) {
        apr_setsocketopt(p, APR_SO_TIMEOUT, timeout);
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
    ap_bucket_make_heap(a, buf, *len, 0, NULL);  /* XXX: check for failure? */
    /*
     * If there's more to read we have to keep the rest of the socket
     * for later. XXX: Note that more complicated bucket types that
     * refer to data not in memory and must therefore have a read()
     * function similar to this one should be wary of copying this
     * code because if they have a destroy function they probably
     * want to migrate the bucket's subordinate structure from the
     * old bucket to a raw new one and adjust it as appropriate,
     * rather than destroying the old one and creating a completely
     * new bucket.
     *
     * Even if there is nothing more to read, don't close the socket here
     * as we have to use it to send any response :)  We could shut it 
     * down for reading, but there is no benefit to doing so.
     */
    if (*len > 0) {
        b = ap_bucket_create_socket(p);
	AP_BUCKET_INSERT_AFTER(a, b);
    }
    return APR_SUCCESS;
}

APU_DECLARE(ap_bucket *) ap_bucket_make_socket(ap_bucket *b, apr_socket_t *p)
{
    /*
     * XXX: We rely on a cleanup on some pool or other to actually
     * destroy the socket. We should probably explicitly call apr to
     * destroy it instead.
     *
     * Note that typically the socket is allocated from the connection pool
     * so it will disappear when the connection is finished. 
     */
    b->type     = &ap_socket_type;
    b->length   = -1;
    b->data     = p;

    return b;
}

APU_DECLARE(ap_bucket *) ap_bucket_create_socket(apr_socket_t *p)
{
    ap_bucket_do_create(ap_bucket_make_socket(b, p));
}

APU_DECLARE_DATA const ap_bucket_type ap_socket_type = {
    "SOCKET", 5,
    ap_bucket_destroy_notimpl,
    socket_read,
    ap_bucket_setaside_notimpl, 
    ap_bucket_split_notimpl,
    ap_bucket_copy_notimpl
};
