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

#ifndef AP_BUCKETS_H
#define AP_BUCKETS_H

#include "apu.h"
#include "apr_network_io.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_mmap.h"
#include "apr_errno.h"
#include "ap_ring.h"
#include "apr.h"
#if APR_HAVE_SYS_UIO_H
#include <sys/uio.h>	/* for struct iovec */
#endif
#if APR_HAVE_STDARG_H
#include <stdarg.h>
#endif

/**
 * @package Bucket Brigades
 */

typedef enum {AP_BLOCK_READ, AP_NONBLOCK_READ} ap_read_type;

/*
 * The one-sentence buzzword-laden overview: Bucket brigades represent
 * a complex data stream that can be passed through a layered IO
 * system without unnecessary copying. A longer overview follows...
 *
 * A bucket brigade is a doubly linked list of buckets, so we
 * aren't limited to inserting at the front and removing at the end.
 * Buckets are only passed around as members of a brigade, although
 * singleton buckets can occur for short periods of time.
 *
 * Buckets are data stores of varous types. They can refer to data in
 * memory, or part of a file or mmap area, or the output of a process,
 * etc. Buckets also have some type-dependent accessor functions:
 * read, split, copy, setaside, and destroy.
 *
 * read returns the address and size of the data in the bucket. If the
 * data isn't in memory then it is read in and the bucket changes type
 * so that it can refer to the new location of the data. If all the
 * data doesn't fit in the bucket then a new bucket is inserted into
 * the brigade to hold the rest of it.
 *
 * split divides the data in a bucket into two regions. After a split
 * the original bucket refers to the first part of the data and a new
 * bucket inserted into the brigade after the original bucket refers
 * to the second part of the data. Reference counts are maintained as
 * necessary.
 *
 * setaside ensures that the data in the bucket has a long enough
 * lifetime. Sometimes it is convenient to create a bucket referring
 * to data on the stack in the expectation that it will be consumed
 * (output to the network) before the stack is unwound. If that
 * expectation turns out not to be valid, the setaside function is
 * called to move the data somewhere safer.
 *
 * copy makes a duplicate of the bucket structure as long as it's
 * possible to have multiple references to a single copy of the
 * data itself.  Not all bucket types can be copied.
 *
 * destroy maintains the reference counts on the resources used by a
 * bucket and frees them if necessary.
 *
 * Note: all of the above functions have wrapper macros (ap_bucket_read(),
 * ap_bucket_destroy(), etc), and those macros should be used rather
 * than using the function pointers directly.
 *
 * To write a bucket brigade, they are first made into an iovec, so that we
 * don't write too little data at one time.  Currently we ignore compacting the
 * buckets into as few buckets as possible, but if we really want good
 * performance, then we need to compact the buckets before we convert to an
 * iovec, or possibly while we are converting to an iovec.
 */

/**
 * Forward declaration of the main types.
 */

typedef struct ap_bucket_brigade ap_bucket_brigade;

typedef struct ap_bucket ap_bucket;

typedef struct ap_bucket_type ap_bucket_type;
struct ap_bucket_type {
    /**
     * The name of the bucket type
     */
    const char *name;
    /** 
     * The number of functions this bucket understands.  Can not be less than
     * five.
     */
    int num_func;
    /**
     * Free the private data and any resources used by the bucket
     *  (if they aren't shared with another bucket).
     * @param data The private data pointer from the bucket to be destroyed
     */
    void (*destroy)(void *data);

    /**
     * Read the data from the bucket. This is guaranteed to be implemented
     *  for all bucket types.
     * @param b The bucket to read from
     * @param str A place to store the data read.  Allocation should only be
     *            done if absolutely necessary. 
     * @param len The amount of data read.
     * @param block Should this read function block if there is more data that
     *              cannot be read immediately.
     * @deffunc apr_status_t read(ap_bucket *b, const char **str, apr_size_t *len, ap_read_type block)
     */
    apr_status_t (*read)(ap_bucket *b, const char **str, apr_size_t *len, ap_read_type block);
    
    /**
     * Make it possible to set aside the data. Buckets containing data that
     *  dies when the stack is un-wound must convert the bucket into a heap
     *  bucket. For most bucket types, though, this is a no-op and this
     *  function will return APR_ENOTIMPL.
     * @param e The bucket to convert
     * @deffunc apr_status_t setaside(ap_bucket *e)
     */
    apr_status_t (*setaside)(ap_bucket *e);

    /**
     * Split one bucket in two at the specified position by duplicating
     *  the bucket structure (not the data) and modifying any necessary
     *  start/end/offset information.  If it's not possible to do this
     *  for the bucket type (perhaps the length of the data is indeterminate,
     *  as with pipe and socket buckets), then APR_ENOTIMPL is returned.
     * @see ap_bucket_split_any().
     * @param e The bucket to split
     * @param point The offset of the first byte in the new bucket
     * @deffunc apr_status_t split(ap_bucket *e, apr_off_t point)
     */
    apr_status_t (*split)(ap_bucket *e, apr_off_t point);

    /**
     * Copy the bucket structure (not the data), assuming that this is
     *  possible for the bucket type. If it's not, APR_ENOTIMPL is returned.
     * @param e The bucket to copy
     * @param c Returns a pointer to the new bucket
     * @deffunc apr_status_t copy
     */
    apr_status_t (*copy)(ap_bucket *e, ap_bucket **c);

};

/**
 * ap_bucket_t structures are allocated on the malloc() heap and
 * their lifetime is controlled by the parent ap_bucket_brigade
 * structure. Buckets can move from one brigade to another e.g. by
 * calling ap_brigade_concat(). In general the data in a bucket has
 * the same lifetime as the bucket and is freed when the bucket is
 * destroyed; if the data is shared by more than one bucket (e.g.
 * after a split) the data is freed when the last bucket goes away.
 */
struct ap_bucket {
    /** Links to the rest of the brigade */
    AP_RING_ENTRY(ap_bucket) link;
    /** The type of bucket.  */
    const ap_bucket_type *type;
    /** The length of the data in the bucket.  This could have been implemented
     *  with a function, but this is an optimization, because the most
     *  common thing to do will be to get the length.  If the length is unknown,
     *  the value of this field will be -1.
     */
    apr_off_t length;
    /** type-dependent data hangs off this pointer */
    void *data;	
};

/** A list of buckets */
struct ap_bucket_brigade {
    /** The pool to associate the brigade with.  The data is not allocated out
     *  of the pool, but a cleanup is registered with this pool.  If the 
     *  brigade is destroyed by some mechanism other than pool destruction,
     *  the destroying function is responsible for killing the cleanup.
     */
    apr_pool_t *p;
    /** The buckets in the brigade are on this list. */
    /*
     * XXX: the ap_bucket_list structure doesn't actually need a name tag
     * because it has no existence independent of struct ap_bucket_brigade;
     * the ring macros are designed so that you can leave the name tag
     * argument empty in this situation but apparently the Windows compiler
     * doesn't like that.
     */
    AP_RING_HEAD(ap_bucket_list, ap_bucket) list;
};

/**
 * Wrappers around the RING macros to reduce the verbosity of the code
 * that handles bucket brigades.
 */
/**
 * Determine if this bucket is the start of the list
 * @param b The bucket to test
 * @return true or false
 * @deffunc int AP_BRIGADE_SENTINEL(ap_bucket *b)
 */
#define AP_BRIGADE_SENTINEL(b)	AP_RING_SENTINEL(&(b)->list, ap_bucket, link)

/**
 * Determine if the bucket brigade is empty
 * @param b The brigade to check
 * @return true or false
 * @deffunc AP_BRIGADE_EMPTY(ap_bucket_brigade *b)
 */
#define AP_BRIGADE_EMPTY(b)	AP_RING_EMPTY(&(b)->list, ap_bucket, link)

/**
 * Return the first bucket in a brigade
 * @param b The brigade to query
 * @return The first bucket in the brigade
 * @deffunc ap_bucket *AP_BUCKET_FIRST(ap_bucket_brigade *b)
 */
#define AP_BRIGADE_FIRST(b)	AP_RING_FIRST(&(b)->list)
/**
 * Return the last bucket in a brigade
 * @param b The brigade to query
 * @return The last bucket in the brigade
 * @deffunc ap_bucket *AP_BUCKET_LAST(ap_bucket_brigade *b)
 */
#define AP_BRIGADE_LAST(b)	AP_RING_LAST(&(b)->list)

/**
 * Iterate through a bucket brigade
 * @param e The current bucket
 * @param b The brigade to iterate over
 * @tip This is the same as either:
 * <pre>
 *		e = AP_BUCKET_FIRST(b);
 * 		while (!AP_BUCKET_SENTINEL(e)) {
 *		    ...
 * 		    e = AP_BUCKET_NEXT(e);
 * 		}
 *     OR
 * 		for (e = AP_BUCKET_FIRST(b); !AP_BUCKET_SENTINEL(e); e = AP_BUCKET_NEXT(e)) {
 *		    ...
 * 		}
 * @deffunc void AP_BRIGADE_FOREACH(ap_bucket *e, ap_bucket_brigade *b)
 */
#define AP_BRIGADE_FOREACH(e, b)					\
	AP_RING_FOREACH((e), &(b)->list, ap_bucket, link)

/**
 * Insert a list of buckets at the front of a brigade
 * @param b The brigade to add to
 * @param e The first bucket in a list of buckets to insert
 * @deffunc void AP_BRIGADE_INSERT_HEAD(ap_bucket_brigade *b, ap_bucket *e)
 */
#define AP_BRIGADE_INSERT_HEAD(b, e)					\
	AP_RING_INSERT_HEAD(&(b)->list, (e), ap_bucket, link)
/**
 * Insert a list of buckets at the end of a brigade
 * @param b The brigade to add to
 * @param e The first bucket in a list of buckets to insert
 * @deffunc void AP_BRIGADE_INSERT_HEAD(ap_bucket_brigade *b, ap_bucket *e)
 */
#define AP_BRIGADE_INSERT_TAIL(b, e)					\
	AP_RING_INSERT_TAIL(&(b)->list, (e), ap_bucket, link)

/**
 * Concatenate two brigades together
 * @param a The first brigade
 * @param b The second brigade
 * @deffunc void AP_BRIGADE_CONCAT(ap_bucket_brigade *a, ap_bucket_brigade *b)
 */
#define AP_BRIGADE_CONCAT(a, b)						\
	AP_RING_CONCAT(&(a)->list, &(b)->list, ap_bucket, link)

/**
 * Insert a list of buckets before a specified bucket
 * @param a The buckets to insert
 * @param b The bucket to insert before
 * @deffunc void AP_BUCKET_INSERT_BEFORE(ap_bucket *a, ap_bucket *b)
 */
#define AP_BUCKET_INSERT_BEFORE(a, b)					\
	AP_RING_INSERT_BEFORE((a), (b), link)
/**
 * Insert a list of buckets after a specified bucket
 * @param a The buckets to insert
 * @param b The bucket to insert after
 * @deffunc void AP_BUCKET_INSERT_AFTER(ap_bucket *a, ap_bucket *b)
 */
#define AP_BUCKET_INSERT_AFTER(a, b)					\
	AP_RING_INSERT_AFTER((a), (b), link)

/**
 * Get the next bucket in the list
 * @param e The current bucket
 * @return The next bucket
 * @deffunc ap_bucket *AP_BUCKET_NEXT(ap_bucket *e)
 */
#define AP_BUCKET_NEXT(e)	AP_RING_NEXT((e), link)
/**
 * Get the previous bucket in the list
 * @param e The current bucket
 * @return The previous bucket
 * @deffunc ap_bucket *AP_BUCKET_PREV(ap_bucket *e)
 */
#define AP_BUCKET_PREV(e)	AP_RING_PREV((e), link)

/**
 * Remove a bucket from its bucket brigade
 * @param e The bucket to remove
 * @deffunc void AP_BUCKET_REMOVE(ap_bucket *e)
 */
#define AP_BUCKET_REMOVE(e)	AP_RING_REMOVE((e), link)

/**
 * Determine if a bucket is a FLUSH bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_FLUSH(ap_bucket *e)
 */
#define AP_BUCKET_IS_FLUSH(e)       (e->type == &ap_flush_type)
/**
 * Determine if a bucket is an EOS bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_EOS(ap_bucket *e)
 */
#define AP_BUCKET_IS_EOS(e)         (e->type == &ap_eos_type)
/**
 * Determine if a bucket is a FILE bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_FILE(ap_bucket *e)
 */
#define AP_BUCKET_IS_FILE(e)        (e->type == &ap_file_type)
/**
 * Determine if a bucket is a PIPE bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_PIPE(ap_bucket *e)
 */
#define AP_BUCKET_IS_PIPE(e)        (e->type == &ap_pipe_type)
/**
 * Determine if a bucket is a SOCKET bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_SOCKET(ap_bucket *e)
 */
#define AP_BUCKET_IS_SOCKET(e)      (e->type == &ap_socket_type)
/**
 * Determine if a bucket is a HEAP bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_HEAP(ap_bucket *e)
 */
#define AP_BUCKET_IS_HEAP(e)        (e->type == &ap_heap_type)
/**
 * Determine if a bucket is a TRANSIENT bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_TRANSIENT(ap_bucket *e)
 */
#define AP_BUCKET_IS_TRANSIENT(e)   (e->type == &ap_transient_type)
/**
 * Determine if a bucket is a IMMORTAL bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_IMMORTAL(ap_bucket *e)
 */
#define AP_BUCKET_IS_IMMORTAL(e)    (e->type == &ap_immortal_type)
/**
 * Determine if a bucket is a MMAP bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_MMAP(ap_bucket *e)
 */
#define AP_BUCKET_IS_MMAP(e)        (e->type == &ap_mmap_type)
/**
 * Determine if a bucket is a POOL bucket
 * @param e The bucket to inspect
 * @return true or false
 * @deffunc int AP_BUCKET_IS_POOL(ap_bucket *e)
 */
#define AP_BUCKET_IS_POOL(e)        (e->type == &ap_pool_type)

/*
 * General-purpose reference counting for the varous bucket types.
 *
 * Any bucket type that keeps track of the resources it uses (i.e.
 * most of them except for IMMORTAL, TRANSIENT, and EOS) needs to
 * attach a reference count to the resource so that it can be freed
 * when the last bucket that uses it goes away. Resource-sharing may
 * occur because of bucket splits or buckets that refer to globally
 * cached data. */

typedef struct ap_bucket_refcount ap_bucket_refcount;
/**
 * The structure used to manage the shared resource must start with an
 * ap_bucket_refcount which is updated by the general-purpose refcount
 * code. A pointer to the bucket-type-dependent private data structure
 * can be cast to a pointer to an ap_bucket_refcount and vice versa.
 */
struct ap_bucket_refcount {
    /** The number of references to this bucket */
    int          refcount;
};

typedef struct ap_bucket_shared ap_bucket_shared;
/**
 * The data pointer of a refcounted bucket points to an
 * ap_bucket_shared structure which describes the region of the shared
 * object that this bucket refers to. The ap_bucket_shared isn't a
 * fully-fledged bucket type: it is a utility type that proper bucket
 * types are based on.
 */
struct ap_bucket_shared {
    /** start of the data in the bucket relative to the private base pointer */
    apr_off_t start;
    /** end of the data in the bucket relative to the private base pointer */
    apr_off_t end;
    /** pointer to the real private data of the bucket,
     * which starts with an ap_bucket_refcount */
    void *data;
};

/*  *****  Non-reference-counted bucket types  *****  */


typedef struct ap_bucket_simple ap_bucket_simple;
/**
 * TRANSIENT and IMMORTAL buckets don't have much to do with looking
 * after the memory that they refer to so they share a lot of their
 * implementation.
 */
struct ap_bucket_simple {
    /** The start of the data in the bucket */
    const char    *start;
    /** The end of the data in the bucket */
    const char    *end;
};

typedef struct ap_bucket_pool ap_bucket_pool;
/**
 * A bucket referring to data allocated out of a pool
 */
struct ap_bucket_pool {
    /** Number of buckets using this memory */
    ap_bucket_refcount  refcount;
    /** The start of the data actually allocated.  This should never be
     * modified, it is only used to free the bucket.
     */
    const char *base;
    /** The pool the data was allocated out of */
    apr_pool_t  *p;
    /** This is a hack, because we call ap_destroy_bucket with the ->data
     *  pointer, so the pool cleanup needs to be registered with that pointer,
     *  but the whole point of the cleanup is to convert the bucket to another
     *  type.  To do that conversion, we need a pointer to the bucket itself.
     *  This gives us a pointer to the original bucket.
     */
    ap_bucket *b;
};

/*  *****  Reference-counted bucket types  *****  */


typedef struct ap_bucket_heap ap_bucket_heap;
/**
 * A bucket referring to data allocated off the heap.
 */
struct ap_bucket_heap {
    /** Number of buckets using this memory */
    ap_bucket_refcount  refcount;
    /** The start of the data actually allocated.  This should never be
     * modified, it is only used to free the bucket.
     */
    char    *base;
    /** how much memory was allocated.  This may not be necessary */
    size_t  alloc_len;
};

typedef struct ap_bucket_mmap ap_bucket_mmap;
/**
 * A bucket referring to an mmap()ed file
 */
struct ap_bucket_mmap {
    /** Number of buckets using this memory */
    ap_bucket_refcount  refcount;
    /** The mmap this sub_bucket refers to */
    apr_mmap_t *mmap;
};

typedef struct ap_bucket_file ap_bucket_file;
/**
 * A bucket referring to an file
 */
struct ap_bucket_file {
    /** The file this bucket refers to */
    apr_file_t *fd;
    /** The offset into the file */
    apr_off_t offset;
};

/*  *****  Bucket Brigade Functions  *****  */
/**
 * Create a new bucket brigade.  The bucket brigade is originally empty.
 * @param The pool to associate with the brigade.  Data is not allocated out
 *        of the pool, but a cleanup is registered.
 * @return The empty bucket brigade
 * @deffunc ap_bucket_brigade *ap_brigade_create(apr_pool_t *p)
 */
APU_DECLARE(ap_bucket_brigade *) ap_brigade_create(apr_pool_t *p);

/**
 * destroy an entire bucket brigade.  This includes destroying all of the
 * buckets within the bucket brigade's bucket list. 
 * @param b The bucket brigade to destroy
 * @deffunc apr_status_t ap_brigade_destroy(ap_bucket_brigade *b)
 */
APU_DECLARE(apr_status_t) ap_brigade_destroy(ap_bucket_brigade *b);

/**
 * Split a bucket brigade into two, such that the given bucket is the
 * first in the new bucket brigade. This function is useful when a
 * filter wants to pass only the initial part of a brigade to the next
 * filter.
 * @param b The brigade to split
 * @param e The first element of the new brigade
 * @return The new brigade
 * @deffunc ap_bucket_brigade *ap_brigade_split(ap_bucket_brigade *b, ap_bucket *e)
 */
APU_DECLARE(ap_bucket_brigade *) ap_brigade_split(ap_bucket_brigade *b,
						 ap_bucket *e);

/**
 * Partition a bucket brigade at a given offset (in bytes from the start of
 * the brigade).  This is useful whenever a filter wants to use known ranges
 * of bytes from the brigade; the ranges can even overlap.
 * @param b The brigade to partition
 * @param point The offset at which to partition the brigade
 * @return A pointer to the first bucket after the partition;
 *         or NULL in any error condition (including partition past the end)
 * @deffunc ap_bucket *ap_brigade_partition(ap_bucket_brigade *b, apr_off_t point)
 */
APU_DECLARE(ap_bucket *) ap_brigade_partition(ap_bucket_brigade *b, apr_off_t point);

#if APR_NOT_DONE_YET
/**
 * consume nbytes from beginning of b -- call ap_bucket_destroy as
 * appropriate, and/or modify start on last element 
 * @param b The brigade to consume data from
 * @param nbytes The number of bytes to consume
 * @deffunc void ap_brigade_consume(ap_bucket_brigade *b, int nbytes)
 */
APU_DECLARE(void) ap_brigade_consume(ap_bucket_brigade *b, int nbytes);
#endif

/**
 * create an iovec of the elements in a bucket_brigade... return number 
 * of elements used.  This is useful for writing to a file or to the
 * network efficiently.
 * @param The bucket brigade to create the iovec out of
 * @param The iovec to create
 * @param The number of elements in the iovec
 * @return The number of iovec elements actually filled out.
 * @deffunc int ap_brigade_to_iovec(ap_bucket_brigade *b, struct iovec *vec, int nvec);
 */
APU_DECLARE(int) ap_brigade_to_iovec(ap_bucket_brigade *b, 
				    struct iovec *vec, int nvec);

/**
 * This function writes a list of strings into a bucket brigade.  We just 
 * allocate a new heap bucket for each string.
 * @param b The bucket brigade to add to
 * @param va A list of strings to add
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va)
 */
APU_DECLARE(int) ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va);

/**
 * This function writes an unspecified number of strings into a bucket brigade.
 * We just allocate a new heap bucket for each string.
 * @param b The bucket brigade to add to
 * @param ... The strings to add
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_putstrs(ap_bucket_brigade *b, ...)
 */
APU_DECLARE_NONSTD(int) ap_brigade_putstrs(ap_bucket_brigade *b, ...);

/**
 * Evaluate a printf and put the resulting string into a bucket at the end 
 * of the bucket brigade.
 * @param b The brigade to write to
 * @param fmt The format of the string to write
 * @param ... The arguments to fill out the format
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...) 
 */
APU_DECLARE_NONSTD(int) ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...);

/**
 * Evaluate a printf and put the resulting string into a bucket at the end 
 * of the bucket brigade.
 * @param b The brigade to write to
 * @param fmt The format of the string to write
 * @param va The arguments to fill out the format
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va) 
 */
APU_DECLARE(int) ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va);


/*  *****  Bucket Functions  *****  */
/**
 * Initialize the core implemented bucket types.  Once this is done,
 * it is possible to add new bucket types to the server
 * @param p The pool to allocate the array out of.
 * @deffunc void ap_init_bucket_types(apr_pool_t *p)
 */
void ap_init_bucket_types(apr_pool_t *p);

/**
 * free the resources used by a bucket. If multiple buckets refer to
 * the same resource it is freed when the last one goes away.
 * @param e The bucket to destroy
 * @deffunc void ap_bucket_destroy(ap_bucket *e)
 */
#define ap_bucket_destroy(e) \
    { \
    e->type->destroy(e->data); \
    free(e); \
    }

/**
 * read the data from the bucket
 * @param e The bucket to read from
 * @param str The location to store the data in
 * @param len The amount of data read
 * @param block Whether the read function blocks
 * @deffunc apr_status_t ap_bucket_read(ap_bucket *e, const char **str, apr_size_t *len, ap_read_type block)
 */
#define ap_bucket_read(e,str,len,block) e->type->read(e, str, len, block)

/**
 * Setaside data so that stack data is not destroyed on returning from
 * the function
 * @param e The bucket to setaside
 * @deffunc apr_status_t ap_bucket_setaside(ap_bucket *e)
 */
#define ap_bucket_setaside(e) e->type->setaside(e)

/**
 * Split one bucket in two.
 * @param e The bucket to split
 * @param point The offset to split the bucket at
 * @deffunc apr_status_t ap_bucket_split(ap_bucket *e, apr_off_t point)
 */
#define ap_bucket_split(e,point) e->type->split(e, point)

/**
 * Copy a bucket.
 * @param e The bucket to copy
 * @param c Returns a pointer to the new bucket
 * @deffunc apr_status_t ap_bucket_copy(ap_bucket *e, ap_bucket **c)
 */
#define ap_bucket_copy(e,c) e->type->copy(e, c)

/* Bucket type handling */

/**
 * A place holder function that signifies that the setaside function was not
 * implemented for this bucket
 * @param data The bucket to setaside
 * @return APR_ENOTIMPL
 * @deffunc apr_status_t ap_bucket_setaside_notimpl(ap_bucket *data)
 */ 
APU_DECLARE_NONSTD(apr_status_t) ap_bucket_setaside_notimpl(ap_bucket *data);
/**
 * A place holder function that signifies that the split function was not
 * implemented for this bucket
 * @param data The bucket to split
 * @param point The location to split the bucket
 * @return APR_ENOTIMPL
 * @deffunc apr_status_t ap_bucket_split_notimpl(ap_bucket *data)
 */ 
APU_DECLARE_NONSTD(apr_status_t) ap_bucket_split_notimpl(ap_bucket *data, 
                                                 apr_off_t point);
/**
 * A place holder function that signifies that the copy function was not
 * implemented for this bucket
 * @param e The bucket to copy
 * @param c Returns a pointer to the new bucket
 * @return APR_ENOTIMPL
 * @deffunc apr_status_t ap_bucket_copy_notimpl(ap_bucket *e, ap_bucket **c)
 */
APU_DECLARE_NONSTD(apr_status_t) ap_bucket_copy_notimpl(ap_bucket *e,
                                                        ap_bucket **c);
/**
 * A place holder function that signifies that the destroy function was not
 * implemented for this bucket
 * @param data The bucket to destroy
 * @deffunc void ap_bucket_destroy(ap_bucket *data)
 */ 
APU_DECLARE_NONSTD(void) ap_bucket_destroy_notimpl(void *data);
/* There is no ap_bucket_read_notimpl, because it is a required function
 */

/**
 * Register a new bucket type
 * @param type The new bucket type to register
 * @return The offset into the array in which the bucket types are stored
 */
int ap_insert_bucket_type(const ap_bucket_type *type);

/* All of the bucket types implemented by the core */
/**
 * The flush bucket type.  This signifies that all data should be flushed to
 * the next filter.  The flush bucket should be sent with the other buckets.
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_flush_type;
/**
 * The EOS bucket type.  This signifies that there will be no more data, ever.
 * All filters MUST send all data to the next filter when they receive a
 * bucket of this type
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_eos_type;
/**
 * The FILE bucket type.  This bucket represents a file on disk
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_file_type;
/**
 * The HEAP bucket type.  This bucket represents a data allocated out of the
 * heap.
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_heap_type;
/**
 * The MMAP bucket type.  This bucket represents an MMAP'ed file
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_mmap_type;
/**
 * The POOL bucket type.  This bucket represents a data that was allocated
 * out of a pool.  IF this bucket is still available when the pool is cleared,
 * the data is copied on to the heap.
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_pool_type;
/**
 * The PIPE bucket type.  This bucket represents a pipe to another program.
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_pipe_type;
/**
 * The IMMORTAL bucket type.  This bucket represents a segment of data that
 * the creator is willing to take responsability for.  The core will do
 * nothing with the data in an immortal bucket
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_immortal_type;
/**
 * The TRANSIENT bucket type.  This bucket represents a data allocated off
 * the stack.  When the setaside function is called, this data is copied on
 * to the heap
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_transient_type;
/**
 * The SOCKET bucket type.  This bucket represents a socket to another machine
 */
APU_DECLARE_DATA extern const ap_bucket_type ap_socket_type;


/*  *****  Shared reference-counted buckets  *****  */

/**
 * Initialize a bucket containing reference-counted data that may be
 * shared. The caller must allocate the bucket if necessary and
 * initialize its type-dependent fields, and allocate and initialize
 * its own private data structure. This function should only be called
 * by type-specific bucket creation functions.
 * @param b The bucket to initialize,
 *          or NULL if a new one should be allocated
 * @param data A pointer to the private data structure
 *             with the reference count at the start
 * @param start The start of the data in the bucket
 *              relative to the private base pointer
 * @param end The end of the data in the bucket
 *            relative to the private base pointer
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_shared(ap_bucket_refcount *r, apr_off_t start, apr_off_t end) 
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_shared(ap_bucket *b, void *data,
					      apr_off_t start, apr_off_t end);

/**
 * Decrement the refcount of the data in the bucket and free the
 * ap_bucket_shared structure. This function should only be called by
 * type-specific bucket destruction functions.
 * @param data The private data pointer from the bucket to be destroyed
 * @return NULL if nothing needs to be done,
 *         otherwise a pointer to the private data structure which
 *         must be destroyed because its reference count is zero
 * @deffunc void *ap_bucket_destroy_shared(ap_bucket *b)
 */
APU_DECLARE(void *) ap_bucket_destroy_shared(void *data);

/**
 * Split a bucket into two at the given point, and adjust the refcount
 * to the underlying data. Most reference-counting bucket types will
 * be able to use this function as their split function without any
 * additional type-specific handling.
 * @param b The bucket to be split
 * @param point The offset of the first byte in the new bucket
 * @return APR_EINVAL if the point is not within the bucket;
 *         APR_ENOMEM if allocation failed;
 *         or APR_SUCCESS
 * @deffunc apr_status_t ap_bucket_split_shared(ap_bucket *b, apr_off_t point)
 */
APU_DECLARE_NONSTD(apr_status_t) ap_bucket_split_shared(ap_bucket *b, apr_off_t point);

/**
 * Copy a refcounted bucket, incrementing the reference count. Most
 * reference-counting bucket types will be able to use this function
 * as their copy function without any additional type-specific handling.
 * @param a The bucket to copy
 * @param c Returns a pointer to the new bucket
 * @return APR_ENOMEM if allocation failed;
           or APR_SUCCESS
 * @deffunc apr_status_t ap_bucket_copy_shared(ap_bucket *a, ap_bucket **c)
 */
APU_DECLARE_NONSTD(apr_status_t) ap_bucket_copy_shared(ap_bucket *a, ap_bucket **c);


/*  *****  Functions to Create Buckets of varying type  *****  */
/*
 * Each bucket type foo has two initialization functions:
 * ap_bucket_make_foo which sets up some already-allocated memory as a
 * bucket of type foo; and ap_bucket_create_foo which allocates memory
 * for the bucket, calls ap_bucket_make_foo, and initializes the
 * bucket's list pointers. The ap_bucket_make_foo functions are used
 * inside the bucket code to change the type of buckets in place;
 * other code should call ap_bucket_create_foo. All the initialization
 * functions change nothing if they fail.
 */

/*
 * This macro implements the guts of ap_bucket_create_foo
 */
#define ap_bucket_do_create(do_make)		\
    do {					\
	ap_bucket *b, *ap__b;			\
	b = calloc(1, sizeof(*b));		\
	if (b == NULL) {			\
	    return NULL;			\
	}					\
	ap__b = do_make;			\
	if (ap__b == NULL) {			\
	    free(b);				\
	    return NULL;			\
	}					\
	AP_RING_ELEM_INIT(ap__b, link);		\
	return ap__b;				\
    } while(0)


/**
 * Create an End of Stream bucket.  This indicates that there is no more data
 * coming from down the filter stack.  All filters should flush at this point.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_eos(void)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_eos(void);
/**
 * Make the bucket passed in an EOS bucket.  This indicates that there is no 
 * more data coming from down the filter stack.  All filters should flush at 
 * this point.
 * @param b The bucket to make into an EOS bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_eos(ap_bucket *b)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_eos(ap_bucket *b);

/**
 * Create a flush  bucket.  This indicates that filters should flush their
 * data.  There is no guarantee that they will flush it, but this is the
 * best we can do.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_flush(void)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_flush(void);
/**
 * Make the bucket passed in a FLUSH  bucket.  This indicates that filters 
 * should flush their data.  There is no guarantee that they will flush it, 
 * but this is the best we can do.
 * @param b The bucket to make into a FLUSH bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_flush(ap_bucket *b)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_flush(ap_bucket *b);

/**
 * Create a bucket referring to long-lived data.
 * @param buf The data to insert into the bucket
 * @param nbyte The size of the data to insert.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_immortal(const char *buf, apr_size_t nbyte, apr_size_t *w)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_immortal(
		const char *buf, apr_size_t nbyte);
/**
 * Make the bucket passed in a bucket refer to long-lived data
 * @param b The bucket to make into a IMMORTAL bucket
 * @param buf The data to insert into the bucket
 * @param nbyte The size of the data to insert.
 * @param w The number of bytes added to the bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_immortal(ap_bucket *b, const char *buf, apr_size_t nbyte, apr_size_t *w)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_immortal(ap_bucket *b,
		const char *buf, apr_size_t nbyte);

/**
 * Create a bucket referring to data on the stack.
 * @param buf The data to insert into the bucket
 * @param nbyte The size of the data to insert.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_transient(const char *buf, apr_size_t nbyte, apr_size_t *w)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_transient(
		const char *buf, apr_size_t nbyte);
/**
 * Make the bucket passed in a bucket refer to stack data
 * @param b The bucket to make into a TRANSIENT bucket
 * @param buf The data to insert into the bucket
 * @param nbyte The size of the data to insert.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_transient(ap_bucket *b, const char *buf, apr_size_t nbyte)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_transient(ap_bucket *b,
		const char *buf, apr_size_t nbyte);

/**
 * Create a bucket referring to memory on the heap. If the caller asks
 * for the data to be copied, this function always allocates 4K of
 * memory so that more data can be added to the bucket without
 * requiring another allocation. Therefore not all the data may be put
 * into the bucket. If copying is not requested then the bucket takes
 * over responsibility for free()ing the memory.
 * @param buf The buffer to insert into the bucket
 * @param nbyte The size of the buffer to insert.
 * @param copy Whether to copy the data into newly-allocated memory or not
 * @param w The number of bytes actually copied into the bucket.
 *          If copy is zero then this return value can be ignored by passing a NULL pointer.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_heap(const char *buf, apr_size_t nbyte, int copy, apr_size_t *w)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_heap(
		const char *buf, apr_size_t nbyte, int copy, apr_size_t *w);
/**
 * Make the bucket passed in a bucket refer to heap data
 * @param b The bucket to make into a HEAP bucket
 * @param buf The buffer to insert into the bucket
 * @param nbyte The size of the buffer to insert.
 * @param copy Whether to copy the data into newly-allocated memory or not
 * @param w The number of bytes actually copied into the bucket.
 *          If copy is zero then this return value can be ignored by passing a NULL pointer.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_heap(ap_bucket *b, const char *buf, apr_size_t nbyte, int copy, apr_size_t *w)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_heap(ap_bucket *b,
		const char *buf, apr_size_t nbyte, int copy, apr_size_t *w);

/**
 * Create a bucket referring to memory allocated out of a pool.
 * @param buf The buffer to insert into the bucket
 * @param p The pool the memory was allocated out of
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_pool(const char *buf, apr_size_t *length, apr_pool_t *p)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_pool(const char *buf,  
                                            apr_size_t length, apr_pool_t *p);
/**
 * Make the bucket passed in a bucket refer to pool data
 * @param b The bucket to make into a pool bucket
 * @param buf The buffer to insert into the bucket
 * @param p The pool the memory was allocated out of
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_pool(ap_bucket *b, const char *buf, apr_size_t *length, apr_pool_t *p)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_pool(ap_bucket *b,
		const char *buf, apr_size_t length, apr_pool_t *p);

/**
 * Create a bucket referring to mmap()ed memory.
 * @param mmap The mmap to insert into the bucket
 * @param start The offset of the first byte in the mmap
 *              that this bucket refers to
 * @param length The number of bytes referred to by this bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_mmap(const apr_mmap_t *mm, apr_size_t start, apr_size_t length)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_mmap(
		apr_mmap_t *mm, apr_off_t start, apr_size_t length);
/**
 * Make the bucket passed in a bucket refer to an MMAP'ed file
 * @param b The bucket to make into a MMAP bucket
 * @param mmap The mmap to insert into the bucket
 * @param start The offset of the first byte in the mmap
 *              that this bucket refers to
 * @param length The number of bytes referred to by this bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_mmap(ap_bucket *b, const apr_mmap_t *mm, apr_size_t start, apr_size_t length)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_mmap(ap_bucket *b,
		apr_mmap_t *mm, apr_off_t start, apr_size_t length);

/**
 * Create a bucket referring to a socket.
 * @param thissocket The socket to put in the bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_socket(apr_socket_t *thissocket)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_socket(apr_socket_t *thissock);
/**
 * Make the bucket passed in a bucket refer to a socket
 * @param b The bucket to make into a SOCKET bucket
 * @param thissocket The socket to put in the bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_socket(ap_bucket *b, apr_socket_t *thissocket)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_socket(ap_bucket *b, apr_socket_t *thissock);

/**
 * Create a bucket referring to a pipe.
 * @param thispipe The pipe to put in the bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_pipe(apr_file_t *thispipe)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_pipe(apr_file_t *thispipe);
/**
 * Make the bucket passed in a bucket refer to a pipe
 * @param b The bucket to make into a PIPE bucket
 * @param thispipe The pipe to put in the bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_pipe(ap_bucket *b, apr_file_t *thispipe)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_pipe(ap_bucket *b, apr_file_t *thispipe);

/**
 * Create a bucket referring to a file.
 * @param fd The file to put in the bucket
 * @param offset The offset where the data of interest begins in the file
 * @param len The amount of data in the file we are interested in
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_file(apr_file_t *fd, apr_off_t offset, apr_size_t len)
 */
APU_DECLARE(ap_bucket *) ap_bucket_create_file(apr_file_t *fd, apr_off_t offset, apr_size_t len);
/**
 * Make the bucket passed in a bucket refer to a file
 * @param b The bucket to make into a FILE bucket
 * @param fd The file to put in the bucket
 * @param offset The offset where the data of interest begins in the file
 * @param len The amount of data in the file we are interested in
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_make_file(ap_bucket *b, apr_file_t *fd, apr_off_t offset, apr_size_t len)
 */
APU_DECLARE(ap_bucket *) ap_bucket_make_file(ap_bucket *b, apr_file_t *fd, 
                                            apr_off_t offset, apr_size_t len);

#endif /* !AP_BUCKETS_H */
