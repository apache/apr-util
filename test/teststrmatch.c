/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002-2003 The Apache Software Foundation.  All rights
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
 *
 */

#include "apr.h"
#include "apr_general.h"
#include "apr_strmatch.h"
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include "apr_want.h"


int main (void)
{
    apr_pool_t *pool;
    const apr_strmatch_pattern *pattern;
    const apr_strmatch_pattern *pattern_nocase;
    const apr_strmatch_pattern *pattern_onechar;
    const apr_strmatch_pattern *pattern_zero;
    const char *input1 = "string that contains a patterN...";
    const char *input2 = "string that contains a pattern...";
    const char *input3 = "pattern at the start of a string";
    const char *input4 = "string that ends with a pattern";
    const char *input5 = "patter\200n not found, negative chars in input";
    const char *input6 = "patter\200n, negative chars, contains pattern...";

    (void) apr_initialize();
    apr_pool_create(&pool, NULL);

    printf("testing pattern precompilation...");
    pattern = apr_strmatch_precompile(pool, "pattern", 1);
    if (!pattern) {
        printf("FAILED\n");
        exit(1);
    }
    pattern_nocase = apr_strmatch_precompile(pool, "pattern", 0);
    if (!pattern_nocase) {
        printf("FAILED\n");
        exit(1);
    }
    pattern_onechar = apr_strmatch_precompile(pool, "g", 0);
    if (!pattern_onechar) {
        printf("FAILED\n");
        exit(1);
    }
    pattern_zero = apr_strmatch_precompile(pool, "", 0);
    if (!pattern_zero) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing invalid match...");
    if (apr_strmatch(pattern, input1, strlen(input1)) != NULL) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing valid match...");
    if (apr_strmatch(pattern, input2, strlen(input2)) != input2 + 23) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing single-character match...");
    if (apr_strmatch(pattern_onechar, input1, strlen(input1)) != input1 + 5) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing zero-length pattern...");
    if (apr_strmatch(pattern_zero, input1, strlen(input1)) != input1) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing inexact-case match...");
    if (apr_strmatch(pattern_nocase, input1, strlen(input1)) != input1 + 23) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing match at start of string...");
    if (apr_strmatch(pattern, input3, strlen(input3)) != input3) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing match at end of string...");
    if (apr_strmatch(pattern, input4, strlen(input4)) != input4 + 24) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing invalid match with negative chars in input string...");
    if (apr_strmatch(pattern, input5, strlen(input5)) != NULL) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    printf("testing valid match with negative chars in input string...");
    if (apr_strmatch(pattern, input6, strlen(input6)) != input6 + 35) {
        printf("FAILED\n");
        exit(1);
    }
    printf("OK\n");

    exit(0);
}
