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
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "apu.h"
#include "apr_portable.h"
#include "apr_thread_mutex.h"
#include "apr_thread_cond.h"
#include "apr_errno.h"
#include "apr_queue.h"

#if APR_HAS_THREADS
/* 
 * define this to get debug messages
 *
#define QUEUE_DEBUG
 */

struct apr_queue_t {
    void              **data;
    unsigned int        nelts; /**< # elements */
    unsigned int        in;    /**< next empty location */
    unsigned int        out;   /**< next filled location */
    unsigned int        bounds;/**< max size of queue */
    unsigned int        full_waiters;
    unsigned int        empty_waiters;
    apr_thread_mutex_t *one_big_mutex;
    apr_thread_cond_t  *not_empty;
    apr_thread_cond_t  *not_full;
    int                 terminated;
};

#ifdef QUEUE_DEBUG
static void Q_DBG(char*msg, apr_queue_t *q) {
    fprintf(stderr, "%ld\t#%d in %d out %d\t%s\n", 
                    apr_os_thread_current(),
                    q->nelts, q->in, q->out,
                    msg
                    );
}
#else
#define Q_DBG(x,y) 
#endif

/**
 * Detects when the apr_queue_t is full. This utility function is expected
 * to be called from within critical sections, and is not threadsafe.
 */
#define apr_queue_full(queue) ((queue)->nelts == (queue)->bounds)

/**
 * Detects when the apr_queue_t is empty. This utility function is expected
 * to be called from within critical sections, and is not threadsafe.
 */
#define apr_queue_empty(queue) ((queue)->nelts == 0)

/**
 * Callback routine that is called to destroy this
 * apr_queue_t when its pool is destroyed.
 */
static apr_status_t queue_destroy(void *data) 
{
    apr_queue_t *queue = data;

    /* Ignore errors here, we can't do anything about them anyway. */

    apr_thread_cond_destroy(queue->not_empty);
    apr_thread_cond_destroy(queue->not_full);
    apr_thread_mutex_destroy(queue->one_big_mutex);

    return APR_SUCCESS;
}

/**
 * Initialize the apr_queue_t.
 */
APU_DECLARE(apr_status_t) apr_queue_create(apr_queue_t **q, 
                                           unsigned int queue_capacity, 
                                           apr_pool_t *a)
{
    apr_status_t rv;
    apr_queue_t *queue;
    queue = apr_palloc(a, sizeof(apr_queue_t));
    *q = queue;

    /* nested doesn't work ;( */
    rv = apr_thread_mutex_create(&queue->one_big_mutex,
                                 APR_THREAD_MUTEX_UNNESTED,
                                 a);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_thread_cond_create(&queue->not_empty, a);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_thread_cond_create(&queue->not_full, a);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    /* Set all the data in the queue to NULL */
    queue->data = apr_pcalloc(a, queue_capacity * sizeof(void*));
    queue->bounds = queue_capacity;
    queue->nelts = 0;
    queue->in = 0;
    queue->out = 0;
    queue->terminated = 0;
    queue->full_waiters = 0;
    queue->empty_waiters = 0;

    apr_pool_cleanup_register(a, queue, queue_destroy, apr_pool_cleanup_null);

    return APR_SUCCESS;
}

/**
 * Push new data onto the queue. Blocks if the queue is full. Once
 * the push operation has completed, it signals other threads waiting
 * in apr_queue_pop() that they may continue consuming sockets.
 */
APU_DECLARE(apr_status_t) apr_queue_push(apr_queue_t *queue, void *data)
{
    apr_status_t rv;

    if (queue->terminated) {
        return APR_EOF; /* no more elements ever again */
    }

    rv = apr_thread_mutex_lock(queue->one_big_mutex);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    if (apr_queue_full(queue)) {
        if (!queue->terminated) {
            queue->full_waiters++;
            rv = apr_thread_cond_wait(queue->not_full, queue->one_big_mutex);
            queue->full_waiters--;
            if (rv != APR_SUCCESS) {
                apr_thread_mutex_unlock(queue->one_big_mutex);
                return rv;
            }
        }
        /* If we wake up and it's still empty, then we were interrupted */
        if (apr_queue_full(queue)) {
            Q_DBG("queue full (intr)", queue);
            rv = apr_thread_mutex_unlock(queue->one_big_mutex);
            if (rv != APR_SUCCESS) {
                return rv;
            }
            if (queue->terminated) {
                return APR_EOF; /* no more elements ever again */
            }
            else {
                return APR_EINTR;
            }
        }
    }

    queue->data[queue->in] = data;
    queue->in = (queue->in + 1) % queue->bounds;
    queue->nelts++;

    if (queue->empty_waiters) {
        Q_DBG("sig !empty", queue);
        rv = apr_thread_cond_signal(queue->not_empty);
        if (rv != APR_SUCCESS) {
            apr_thread_mutex_unlock(queue->one_big_mutex);
            return rv;
        }
    }

    rv = apr_thread_mutex_unlock(queue->one_big_mutex);
    return rv;
}

/**
 * Push new data onto the queue. Blocks if the queue is full. Once
 * the push operation has completed, it signals other threads waiting
 * in apr_queue_pop() that they may continue consuming sockets.
 */
APU_DECLARE(apr_status_t) apr_queue_trypush(apr_queue_t *queue, void *data)
{
    apr_status_t rv;

    if (queue->terminated) {
        return APR_EOF; /* no more elements ever again */
    }

    rv = apr_thread_mutex_lock(queue->one_big_mutex);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    if (apr_queue_full(queue)) {
        rv = apr_thread_mutex_unlock(queue->one_big_mutex);
        return APR_EAGAIN;
    }
    
    queue->data[queue->in] = data;
    queue->in = (queue->in + 1) % queue->bounds;
    queue->nelts++;

    if (queue->empty_waiters) {
        Q_DBG("sig !empty", queue);
        rv  = apr_thread_cond_signal(queue->not_empty);
        if (rv != APR_SUCCESS) {
            apr_thread_mutex_unlock(queue->one_big_mutex);
            return rv;
        }
    }

    rv = apr_thread_mutex_unlock(queue->one_big_mutex);
    return rv;
}

/**
 * not thread safe
 */
APU_DECLARE(unsigned int) apr_queue_size(apr_queue_t *queue) {
    return queue->nelts;
}

/**
 * Retrieves the next item from the queue. If there are no
 * items available, it will block until one becomes available.
 * Once retrieved, the item is placed into the address specified by
 * 'data'.
 */
APU_DECLARE(apr_status_t) apr_queue_pop(apr_queue_t *queue, void **data)
{
    apr_status_t rv;

    if (queue->terminated) {
        return APR_EOF; /* no more elements ever again */
    }

    rv = apr_thread_mutex_lock(queue->one_big_mutex);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    /* Keep waiting until we wake up and find that the queue is not empty. */
    if (apr_queue_empty(queue)) {
        if (!queue->terminated) {
            queue->empty_waiters++;
            rv = apr_thread_cond_wait(queue->not_empty, queue->one_big_mutex);
            queue->empty_waiters--;
            if (rv != APR_SUCCESS) {
                apr_thread_mutex_unlock(queue->one_big_mutex);
                return rv;
            }
        }
        /* If we wake up and it's still empty, then we were interrupted */
        if (apr_queue_empty(queue)) {
            Q_DBG("queue empty (intr)", queue);
            rv = apr_thread_mutex_unlock(queue->one_big_mutex);
            if (rv != APR_SUCCESS) {
                return rv;
            }
            if (queue->terminated) {
                return APR_EOF; /* no more elements ever again */
            }
            else {
                return APR_EINTR;
            }
        }
    } 

    *data = queue->data[queue->out];
    queue->nelts--;

    queue->out = (queue->out + 1) % queue->bounds;
    if (queue->full_waiters) {
        Q_DBG("signal !full", queue);
        rv = apr_thread_cond_signal(queue->not_full);
        if (rv != APR_SUCCESS) {
            apr_thread_mutex_unlock(queue->one_big_mutex);
            return rv;
        }
    }

    rv = apr_thread_mutex_unlock(queue->one_big_mutex);
    return rv;
}

/**
 * Retrieves the next item from the queue. If there are no
 * items available, it will block until one becomes available.
 * Once retrieved, the item is placed into the address specified by
 * 'data'.
 */
APU_DECLARE(apr_status_t) apr_queue_trypop(apr_queue_t *queue, void **data)
{
    apr_status_t rv;

    if (queue->terminated) {
        return APR_EOF; /* no more elements ever again */
    }

    rv = apr_thread_mutex_lock(queue->one_big_mutex);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    /* Keep waiting until we wake up and find that the queue is not empty. */
    if (apr_queue_empty(queue)) {
        rv = apr_thread_mutex_unlock(queue->one_big_mutex);
        return APR_EAGAIN;
    } 

    *data = queue->data[queue->out];
    queue->nelts--;

    queue->out = (queue->out + 1) % queue->bounds;
    if (queue->full_waiters) {
        Q_DBG("signal !full", queue);
        rv = apr_thread_cond_signal(queue->not_full);
        if (rv != APR_SUCCESS) {
            apr_thread_mutex_unlock(queue->one_big_mutex);
            return rv;
        }
    }

    rv = apr_thread_mutex_unlock(queue->one_big_mutex);
    return rv;
}

APU_DECLARE(apr_status_t) apr_queue_interrupt_all(apr_queue_t *queue)
{
    apr_status_t rv;
    Q_DBG("intr all", queue);    
    if ((rv = apr_thread_mutex_lock(queue->one_big_mutex)) != APR_SUCCESS) {
        return rv;
    }
    apr_thread_cond_broadcast(queue->not_empty);
    apr_thread_cond_broadcast(queue->not_full);

    if ((rv = apr_thread_mutex_unlock(queue->one_big_mutex)) != APR_SUCCESS) {
        return rv;
    }

    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apr_queue_term(apr_queue_t *queue)
{
    apr_status_t rv;

    if ((rv = apr_thread_mutex_lock(queue->one_big_mutex)) != APR_SUCCESS) {
        return rv;
    }

    /* we must hold one_big_mutex when setting this... otherwise,
     * we could end up setting it and waking everybody up just after a 
     * would-be popper checks it but right before they block
     */
    queue->terminated = 1;
    if ((rv = apr_thread_mutex_unlock(queue->one_big_mutex)) != APR_SUCCESS) {
        return rv;
    }
    return apr_queue_interrupt_all(queue);
}

#endif /* APR_HAS_THREADS */
