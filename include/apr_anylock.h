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

/**
 * @file apr_anylock.h
 * @brief APR-Util transparent any lock flavor wrapper
 */
#ifndef APR_ANYLOCK_H
#define APR_ANYLOCK_H

#include "apr_proc_mutex.h"
#include "apr_thread_mutex.h"
#include "apr_thread_rwlock.h"

/** Structure that may contain any APR lock type */
typedef struct apr_anylock_t {
    /** Indicates what type of lock is in lock */
    enum tm_lock {
        apr_anylock_none,           /**< None */
        apr_anylock_procmutex,      /**< Process-based */
        apr_anylock_threadmutex,    /**< Thread-based */
        apr_anylock_readlock,       /**< Read lock */
        apr_anylock_writelock       /**< Write lock */
    } type;
    /** Union of all possible APR locks */
    union apr_anylock_u_t {
        apr_proc_mutex_t *pm;       /**< Process mutex */
#if APR_HAS_THREADS
        apr_thread_mutex_t *tm;     /**< Thread mutex */
        apr_thread_rwlock_t *rw;    /**< Read-write lock */
#endif
    } lock;
} apr_anylock_t;

#if APR_HAS_THREADS

/** Lock an apr_anylock_t structure */
#define APR_ANYLOCK_LOCK(lck)                \
    (((lck)->type == apr_anylock_none)         \
      ? APR_SUCCESS                              \
      : (((lck)->type == apr_anylock_threadmutex)  \
          ? apr_thread_mutex_lock((lck)->lock.tm)    \
          : (((lck)->type == apr_anylock_procmutex)    \
              ? apr_proc_mutex_lock((lck)->lock.pm)      \
              : (((lck)->type == apr_anylock_readlock)     \
                  ? apr_thread_rwlock_rdlock((lck)->lock.rw) \
                  : (((lck)->type == apr_anylock_writelock)    \
                      ? apr_thread_rwlock_wrlock((lck)->lock.rw) \
                      : APR_EINVAL)))))

#else /* APR_HAS_THREADS */

#define APR_ANYLOCK_LOCK(lck)                \
    (((lck)->type == apr_anylock_none)         \
      ? APR_SUCCESS                              \
          : (((lck)->type == apr_anylock_procmutex)    \
              ? apr_proc_mutex_lock((lck)->lock.pm)      \
                      : APR_EINVAL))

#endif /* APR_HAS_THREADS */

#if APR_HAS_THREADS

/** Try to lock an apr_anylock_t structure */
#define APR_ANYLOCK_TRYLOCK(lck)                \
    (((lck)->type == apr_anylock_none)            \
      ? APR_SUCCESS                                 \
      : (((lck)->type == apr_anylock_threadmutex)     \
          ? apr_thread_mutex_trylock((lck)->lock.tm)    \
          : (((lck)->type == apr_anylock_procmutex)       \
              ? apr_proc_mutex_trylock((lck)->lock.pm)      \
              : (((lck)->type == apr_anylock_readlock)        \
                  ? apr_thread_rwlock_tryrdlock((lck)->lock.rw) \
                  : (((lck)->type == apr_anylock_writelock)       \
                      ? apr_thread_rwlock_trywrlock((lck)->lock.rw) \
                          : APR_EINVAL)))))

#else /* APR_HAS_THREADS */

#define APR_ANYLOCK_TRYLOCK(lck)                \
    (((lck)->type == apr_anylock_none)            \
      ? APR_SUCCESS                                 \
          : (((lck)->type == apr_anylock_procmutex)       \
              ? apr_proc_mutex_trylock((lck)->lock.pm)      \
                          : APR_EINVAL))

#endif /* APR_HAS_THREADS */

#if APR_HAS_THREADS

/** Unlock an apr_anylock_t structure */
#define APR_ANYLOCK_UNLOCK(lck)              \
    (((lck)->type == apr_anylock_none)         \
      ? APR_SUCCESS                              \
      : (((lck)->type == apr_anylock_threadmutex)  \
          ? apr_thread_mutex_unlock((lck)->lock.tm)  \
          : (((lck)->type == apr_anylock_procmutex)    \
              ? apr_proc_mutex_unlock((lck)->lock.pm)    \
              : ((((lck)->type == apr_anylock_readlock) || \
                  ((lck)->type == apr_anylock_writelock))    \
                  ? apr_thread_rwlock_unlock((lck)->lock.rw)   \
                      : APR_EINVAL))))

#else /* APR_HAS_THREADS */

#define APR_ANYLOCK_UNLOCK(lck)              \
    (((lck)->type == apr_anylock_none)         \
      ? APR_SUCCESS                              \
          : (((lck)->type == apr_anylock_procmutex)    \
              ? apr_proc_mutex_unlock((lck)->lock.pm)    \
                      : APR_EINVAL))

#endif /* APR_HAS_THREADS */

#endif /* !APR_ANYLOCK_H */
