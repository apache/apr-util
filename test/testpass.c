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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "apr_errno.h"
#include "apr_strings.h"
#include "apr_file_io.h"
#include "apr_thread_proc.h"
#include "apr_md5.h"

static struct {
    const char *password;
    const char *hash;
} passwords[] =
{
/*
  passwords and hashes created with Apache's htpasswd utility like this:
  
  htpasswd -c -b passwords pass1 pass1
  htpasswd -b passwords pass2 pass2
  htpasswd -b passwords pass3 pass3
  htpasswd -b passwords pass4 pass4
  htpasswd -b passwords pass5 pass5
  htpasswd -b passwords pass6 pass6
  htpasswd -b passwords pass7 pass7
  htpasswd -b passwords pass8 pass8
  (insert Perl one-liner to convert to initializer :) )
 */
    {"pass1", "1fWDc9QWYCWrQ"},
    {"pass2", "1fiGx3u7QoXaM"},
    {"pass3", "1fzijMylTiwCs"},
    {"pass4", "nHUYc8U2UOP7s"},
    {"pass5", "nHpETGLGPwAmA"},
    {"pass6", "nHbsbWmJ3uyhc"},
    {"pass7", "nHQ3BbF0Y9vpI"},
    {"pass8", "nHZA1rViSldQk"}
};
static int num_passwords = sizeof(passwords) / sizeof(passwords[0]);

static void check_rv(apr_status_t rv)
{
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "bailing\n");
        exit(1);
    }
}

static void test(void)
{
    int i;

    for (i = 0; i < num_passwords; i++) {
        apr_status_t rv = apr_password_validate(passwords[i].password,
                                                passwords[i].hash);
        assert(rv == APR_SUCCESS);
    }
}

#if APR_HAS_THREADS

static void * APR_THREAD_FUNC testing_thread(apr_thread_t *thd,
                                             void *data)
{
    int i;

    for (i = 0; i < 100; i++) {
        test();
    }
    return APR_SUCCESS;
}

static void thread_safe_test(apr_pool_t *p)
{
#define NUM_THR 20
    apr_thread_t *my_threads[NUM_THR];
    int i;
    apr_status_t rv;
    
    for (i = 0; i < NUM_THR; i++) {
        rv = apr_thread_create(&my_threads[i], NULL, testing_thread, NULL, p);
        check_rv(rv);
    }

    for (i = 0; i < NUM_THR; i++) {
        apr_thread_join(&rv, my_threads[i]);
    }
}
#endif

int main(void)
{
    apr_status_t rv;
    apr_pool_t *p;

    rv = apr_initialize();
    check_rv(rv);
    rv = apr_pool_create(&p, NULL);
    check_rv(rv);
    atexit(apr_terminate);

    /* before creating any threads, test it first just to check
     * for problems with the test driver
     */
    printf("dry run\n");
    test();

#if APR_HAS_THREADS
    printf("thread-safe test\n");
    thread_safe_test(p);
#endif
    
    return 0;
}
