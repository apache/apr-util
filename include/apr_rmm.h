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

#ifndef APR_RMM_H
#define APR_RMM_H
/** 
 * @file apr_rmm.h
 * @brief APR-UTIL Relocatable Memory Management Routines
 */
/**
 * @defgroup APR_Util_RMM Relocatable Memory Management Routines
 * @ingroup APR_Util
 * @{
 */

#include "apr.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apu.h"
#include "apr_anylock.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Structure to access Relocatable, Managed Memory */
typedef struct apr_rmm_t apr_rmm_t;

/** Fundemental allocation unit, within a spcific apr_rmm_off_t */
typedef apr_size_t   apr_rmm_off_t;

/**
 * Initialize a relocatable memory block to be managed by the apr_rmm API.
 * @param rmm The relocatable memory block
 * @param lock An apr_anylock_t of the appropriate type of lock
 * @param membuf The block of relocateable memory to be managed
 * @param memsize The size of relocateable memory block to be managed
 * @param cont The pool to use for local storage and management
 */
APU_DECLARE(apr_status_t) apr_rmm_init(apr_rmm_t **rmm, apr_anylock_t *lock,
                                       void* membuf, apr_size_t memsize, 
                                       apr_pool_t *cont);

/**
 * Destroy a managed memory block.
 * @param rmm The relocatable memory block to destroy
 */
APU_DECLARE(apr_status_t) apr_rmm_destroy(apr_rmm_t *rmm);

/**
 * Attach to a relocatable memory block already managed by the apr_rmm API.
 * @param rmm The relocatable memory block
 * @param lock An apr_anylock_t of the appropriate type of lock
 * @param membuf The block of relocateable memory already under management
 * @param cont The pool to use for local storage and management
 */
APU_DECLARE(apr_status_t) apr_rmm_attach(apr_rmm_t **rmm, apr_anylock_t *lock,
                                         void* membuf, apr_pool_t *cont);

/**
 * Detach from the managed block of memory.
 * @param rmm The relocatable memory block to detach from
 */
APU_DECLARE(apr_status_t) apr_rmm_detach(apr_rmm_t *rmm);

/**
 * Allocate memory from the block of relocatable memory.
 * @param rmm The relocatable memory block
 * @param reqsize How much memory to allocate
 */
APU_DECLARE(apr_rmm_off_t) apr_rmm_malloc(apr_rmm_t *rmm, apr_size_t reqsize);

/**
 * Realloc memory from the block of relocatable memory.
 * @param rmm The relocatable memory block
 * @param entity The memory allocation to realloc
 * @param reqsize The new size
 */
APU_DECLARE(apr_rmm_off_t) apr_rmm_realloc(apr_rmm_t *rmm, void *entity, apr_size_t reqsize);

/**
 * Allocate memory from the block of relocatable memory and initialize it to zero.
 * @param rmm The relocatable memory block
 * @param reqsize How much memory to allocate
 */
APU_DECLARE(apr_rmm_off_t) apr_rmm_calloc(apr_rmm_t *rmm, apr_size_t reqsize);

/**
 * Free allocation returned by apr_rmm_malloc or apr_rmm_calloc.
 * @param rmm The relocatable memory block
 * @param entity The memory allocation to free
 */
APU_DECLARE(apr_status_t) apr_rmm_free(apr_rmm_t *rmm, apr_rmm_off_t entity);

/**
 * Retrieve the physical address of a relocatable allocation of memory
 * @param rmm The relocatable memory block
 * @param entity The memory allocation to free
 */
APU_DECLARE(void *) apr_rmm_addr_get(apr_rmm_t *rmm, apr_rmm_off_t entity);

/**
 * Compute the offset of a relocatable allocation of memory
 * @param rmm The relocatable memory block
 * @param entity The physical address to convert to an offset
 */
APU_DECLARE(apr_rmm_off_t) apr_rmm_offset_get(apr_rmm_t *rmm, void* entity);

/**
 * Compute the required overallocation of memory needed to fit n allocs
 * @param n The number of alloc/calloc regions desired
 */
APU_DECLARE(apr_size_t) apr_rmm_overhead_get(int n);

#ifdef __cplusplus
}
#endif
/** @} */
#endif  /* ! APR_RMM_H */

