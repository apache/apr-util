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

#ifndef APR_RESLIST_H
#define APR_RESLIST_H

/** 
 * @file apr_reslist.h
 * @brief APR-UTIL Resource List Routines
 */

#include "apr.h"
#include "apu.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_time.h"

#if APR_HAS_THREADS

/**
 * @defgroup APR_Util_RL Resource List Routines
 * @ingroup APR_Util
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Opaque resource list object */
typedef struct apr_reslist_t apr_reslist_t;

/* Generic constructor called by resource list when it needs to create a
 * resource.
 * @param resource opaque resource
 * @param param flags
 * @param pool  Pool
 */
typedef apr_status_t (*apr_reslist_constructor)(void **resource, void *params,
                                                apr_pool_t *pool);

/* Generic destructor called by resource list when it needs to destroy a
 * resource.
 * @param resource opaque resource
 * @param param flags
 * @param pool  Pool
 */
typedef apr_status_t (*apr_reslist_destructor)(void *resource, void *params,
                                               apr_pool_t *pool);

/**
 * Create a new resource list with the following parameters:
 * @param reslist An address where the pointer to the new resource
 *                list will be stored.
 * @param pool The pool to use for local storage and management
 * @param min Allowed minimum number of available resources. Zero
 *            creates new resources only when needed.
 * @param smax Resources will be destroyed to meet this maximum
 *             restriction as they expire.
 * @param hmax Absolute maximum limit on the number of total resources.
 * @param ttl If non-zero, sets the maximum amount of time a resource
 *               may be available while exceeding the soft limit.
 * @param con Constructor routine that is called to create a new resource.
 * @param de Destructor routine that is called to destroy an expired resource.
 * @param params Passed to constructor and deconstructor
 * @param pool The pool from which to create this resoure list. Also the
 *             same pool that is passed to the constructor and destructor
 *             routines.
 */
APU_DECLARE(apr_status_t) apr_reslist_create(apr_reslist_t **reslist,
                                             int min, int smax, int hmax,
                                             apr_interval_time_t ttl,
                                             apr_reslist_constructor con,
                                             apr_reslist_destructor de,
                                             void *params,
                                             apr_pool_t *pool);

/**
 * Destroy the given resource list and all resources controlled by
 * this list.
 * FIXME: Should this block until all resources become available,
 *        or maybe just destroy all the free ones, or maybe destroy
 *        them even though they might be in use by something else?
 * @param reslist The reslist to destroy
 */
APU_DECLARE(apr_status_t) apr_reslist_destroy(apr_reslist_t *reslist);

/**
 * Retrieve a resource from the list, creating a new one if necessary.
 * If we have met our maximum number of resources, we will block
 * until one becomes available.
 */
APU_DECLARE(apr_status_t) apr_reslist_acquire(apr_reslist_t *reslist,
                                              void **resource);

/**
 * Return a resource back to the list of available resources.
 */
APU_DECLARE(apr_status_t) apr_reslist_release(apr_reslist_t *reslist,
                                              void *resource);

#ifdef __cplusplus
}
#endif

/** @} */

#endif  /* APR_HAS_THREADS */

#endif  /* ! APR_RESLIST_H */
