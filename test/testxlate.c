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

#include <stdio.h>
#include <stdlib.h>

#include "apr.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_xlate.h"

static const char test_utf8[] = "Edelwei\xc3\x9f";
static const char test_utf7[] = "Edelwei+AN8-";
static const char test_latin1[] = "Edelwei\xdf";
static const char test_latin2[] = "Edelwei\xdf";


static int check_status (apr_status_t status, const char *msg)
{
    if (status)
    {
        static char buf[1024];
        printf("ERROR: %s\n      %s\n", msg,
               apr_strerror(status, buf, sizeof(buf)));
        return 1;
    }
    return 0;
}

static int test_conversion (apr_xlate_t *convset,
                            const char *inbuf,
                            const char *expected)
{
    static char buf[1024];
    int retcode = 0;
    apr_size_t inbytes_left = strlen(inbuf) + 1;
    apr_size_t outbytes_left = sizeof(buf) - 1;
    apr_status_t status = apr_xlate_conv_buffer(convset,
                                                inbuf,
                                                &inbytes_left,
                                                buf,
                                                &outbytes_left);
    retcode |= check_status(status, "apr_xlate_conv_buffer");
    if ((!status || APR_STATUS_IS_INCOMPLETE(status))
        && strcmp(buf, expected))
    {
        printf("ERROR: expected: '%s'\n       actual:   '%s'"
               "\n       inbytes_left: %"APR_SIZE_T_FMT"\n",
               expected, buf, inbytes_left);
        retcode |= 1;
    }
    return retcode;
}

static int one_test (const char *cs1, const char *cs2,
                     const char *str1, const char *str2,
                     apr_pool_t *pool)
{
    apr_xlate_t *convset;
    const char *msg = apr_psprintf(pool, "apr_xlate_open(%s, %s)", cs2, cs1);
    int retcode = check_status(apr_xlate_open(&convset, cs2, cs1, pool), msg);
    if (!retcode)
    {
        retcode |= test_conversion(convset, str1, str2);
        retcode |= check_status(apr_xlate_close(convset), "apr_xlate_close");
    }
    printf("%s:  %s -> %s\n", (retcode ? "FAIL" : "PASS"), cs1, cs2);
    return retcode;
}


int main (int argc, char **argv)
{
    apr_pool_t *pool;
    int retcode = 0;

#ifndef APR_HAS_XLATE
    puts("SKIP: apr_xlate not implemented");
    return 0;
#endif

    apr_initialize();
    atexit(apr_terminate);
    apr_pool_create(&pool, NULL);

    /* 1. Identity transformation: UTF-8 -> UTF-8 */
    retcode |= one_test("UTF-8", "UTF-8", test_utf8, test_utf8, pool);

    /* 2. UTF-8 <-> ISO-8859-1 */
    retcode |= one_test("UTF-8", "ISO-8859-1", test_utf8, test_latin1, pool);
    retcode |= one_test("ISO-8859-1", "UTF-8", test_latin1, test_utf8, pool);

    /* 3. ISO-8859-1 <-> ISO-8859-2, identity */
    retcode |= one_test("ISO-8859-1", "ISO-8859-2",
                        test_latin1, test_latin2, pool);
    retcode |= one_test("ISO-8859-2", "ISO-8859-1",
                        test_latin2, test_latin1, pool);

    /* 4. Transformation using charset aliases */
    retcode |= one_test("UTF-8", "UTF-7", test_utf8, test_utf7, pool);
    retcode |= one_test("UTF-7", "UTF-8", test_utf7, test_utf8, pool);

    return retcode;
}
