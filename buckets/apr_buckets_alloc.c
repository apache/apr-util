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

#include "apr_buckets.h"
#include "apr_allocator.h"

#define ALLOC_AMT (8192 - APR_MEMNODE_T_SIZE)

typedef struct node_header_t {
    apr_size_t size;
    apr_bucket_alloc_t *alloc;
    apr_memnode_t *memnode;
    struct node_header_t *next;
} node_header_t;

#define SIZEOF_NODE_HEADER_T  APR_ALIGN_DEFAULT(sizeof(node_header_t))
#define SMALL_NODE_SIZE       (APR_BUCKET_ALLOC_SIZE + SIZEOF_NODE_HEADER_T)

/** A list of free memory from which new buckets or private bucket
 *  structures can be allocated.
 */
struct apr_bucket_alloc_t {
    apr_pool_t *pool;
    apr_allocator_t *allocator;
    node_header_t *freelist;
    apr_memnode_t *blocks;
};

static apr_status_t alloc_cleanup(void *data)
{
    apr_bucket_alloc_t *list = data;
    apr_allocator_t *allocator = list->allocator;

    apr_allocator_free(allocator, list->blocks);
    apr_allocator_destroy(allocator);
    return APR_SUCCESS;
}

APU_DECLARE_NONSTD(apr_bucket_alloc_t *) apr_bucket_alloc_create(apr_pool_t *p)
{
    apr_allocator_t *allocator;
    apr_bucket_alloc_t *list;
    apr_memnode_t *block;

    apr_allocator_create(&allocator);
    block = apr_allocator_alloc(allocator, ALLOC_AMT);
    list = (apr_bucket_alloc_t *)block->first_avail;
    list->pool = p;
    list->allocator = allocator;
    list->freelist = NULL;
    list->blocks = block;
    block->first_avail += APR_ALIGN_DEFAULT(sizeof(*list));

    apr_pool_cleanup_register(list->pool, list, alloc_cleanup,
                              apr_pool_cleanup_null);

    return list;
}

APU_DECLARE_NONSTD(void) apr_bucket_alloc_destroy(apr_bucket_alloc_t *list)
{
    apr_pool_cleanup_run(list->pool, list, alloc_cleanup);
}

APU_DECLARE_NONSTD(void *) apr_bucket_alloc(apr_size_t size, 
                                            apr_bucket_alloc_t *list)
{
    node_header_t *node;
    apr_memnode_t *active = list->blocks;
    char *endp;

    size += SIZEOF_NODE_HEADER_T;
    if (size <= SMALL_NODE_SIZE) {
        if (list->freelist) {
            node = list->freelist;
            list->freelist = node->next;
        }
        else {
            endp = active->first_avail + SMALL_NODE_SIZE;
            if (endp >= active->endp) {
                list->blocks = apr_allocator_alloc(list->allocator, ALLOC_AMT);
                list->blocks->next = active;
                active = list->blocks;
                endp = active->first_avail + SMALL_NODE_SIZE;
            }
            node = (node_header_t *)active->first_avail;
            node->alloc = list;
            node->memnode = active;
            node->size = SMALL_NODE_SIZE;
            active->first_avail = endp;
        }
    }
    else {
        apr_memnode_t *memnode = apr_allocator_alloc(list->allocator, size);
        node = (node_header_t *)memnode->first_avail;
        node->alloc = list;
        node->memnode = memnode;
        node->size = size;
    }
    return ((char *)node) + SIZEOF_NODE_HEADER_T;
}

APU_DECLARE_NONSTD(void) apr_bucket_free(void *mem)
{
    node_header_t *node = (node_header_t *)((char *)mem - SIZEOF_NODE_HEADER_T);
    apr_bucket_alloc_t *list = node->alloc;

    if (node->size == SMALL_NODE_SIZE) {
        node->next = list->freelist;
        list->freelist = node;
    }
    else {
        apr_allocator_free(list->allocator, node->memnode);
    }
}
