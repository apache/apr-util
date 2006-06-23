/* Copyright 2000-2006 The Apache Software Foundation or its licensors, as
 * applicable.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* This file came from the SDBM package (written by oz@nexus.yorku.ca).
 * That package was under public domain. This file has been ported to
 * APR, updated to ANSI C and other, newer idioms, and added to the Apache
 * codebase under the above copyright and license.
 */

/*
 * testssl: Simple APR SSL sockets test.
 */

#include "apr.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_getopt.h"
#include "apr_time.h"
#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "apr_ssl.h"
#include "apr_network_io.h"

#include "apu_config.h"

#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>     /* for atexit(), malloc() */
#include <string.h>

int main(int argc, const char * const * argv)
{
    apr_pool_t *pool;
    apr_ssl_factory_t *asf = NULL;
    apr_sockaddr_t *remoteSA;
    apr_status_t rv;
    apr_pollset_t *pollset;

#ifdef APU_HAVE_SSL

    (void) apr_initialize();
    apr_pool_create(&pool, NULL);
    atexit(apr_terminate);

    printf("SSL Library: %s\n", apr_ssl_library_name());

    if (apr_pollset_create(&pollset, 1, pool, 0) != APR_SUCCESS) {
        printf("Failed to create pollset!\n");
        exit(1);
    }

    if (apr_ssl_factory_create(&asf, NULL, NULL, NULL, pool) != APR_SUCCESS) {
        fprintf(stderr, "Unable to create client factory\n");

    } else {
        apr_ssl_socket_t *sslSock;
        fprintf(stdout, "Client factory created\n");
        if (apr_ssl_socket_create(&sslSock, AF_INET, SOCK_STREAM, 0, asf, 
                                  NULL) != APR_SUCCESS) {
            printf("failed to create socket\n");
        } else {
            printf("created ssl socket\n");

            rv = apr_sockaddr_info_get(&remoteSA, "svn.apache.org", 
                                       APR_UNSPEC, 443, 0, pool);
            if (rv == APR_SUCCESS) {
                apr_size_t len = 16;
                char buffer[4096];

                rv = apr_ssl_socket_connect(sslSock, remoteSA);
                printf("Connect = %s\n", (rv == APR_SUCCESS ? "OK" : "Failed"));

                rv = apr_pollset_add_ssl_socket(pollset, sslSock);
                printf("Pollset add = %s\n", (rv == APR_SUCCESS ? "OK" : "Failed"));

                printf("send: %s\n",
                       (apr_ssl_socket_send(sslSock, "GET / HTTP/1.0\n\n", 
                                            &len) == APR_SUCCESS ?
                        "OK" : "Failed"));

                len = 4096;
                printf("recv: %s\n%s\n",
                       (apr_ssl_socket_recv(sslSock, buffer, &len) == 
                          APR_SUCCESS ? "OK" : "Failed"),
                       buffer);
                rv = apr_pollset_remove_ssl_socket(pollset, sslSock);
                printf("Pollset remove = %s\n", (rv == APR_SUCCESS ? "OK" : "Failed"));

            }

            printf("close = %s\n",
                   (apr_ssl_socket_close(sslSock) == APR_SUCCESS ? 
                    "OK" : "Failed"));

        }
    }

    apr_pool_destroy(pool);

#endif /* APU_HAVE_SSL */

    return 0;
}
