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

#include "httpd.h"
#include "apr_general.h"
#include "apr_mmap.h"
#include "apr_errno.h"
#include "ap_ring.h"
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>	/* for struct iovec */
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

/**
 * @package Bucket Brigades
 */

typedef enum {AP_NONBLOCK_READ, AP_BLOCK_READ} ap_read_type;

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
 * read, split, setaside, and destroy.
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
 * destroy maintains the reference counts on the resources used by a
 * bucket and frees them if necessary.
 *
 * To write a bucket brigade, they are first made into an iovec, so that we
 * don't write too little data at one time.  Currently we ignore compacting the
 * buckets into as few buckets as possible, but if we really want good
 * performance, then we need to compact the buckets before we convert to an
 * iovec, or possibly while we are converting to an iovec.
 */

/* The types of bucket brigades the code knows about.  We don't really need
 * this enum.  All access to the bucket brigades is done through function
 * pointers in the bucket type.  
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
     * four.
     */
    int num_func;
    /**
     * Free the private data and any resources used by the bucket
     * (if they aren't shared with another bucket).
     * @param data The private data pointer from the bucket to be destroyed
     */
    void (*destroy)(void *data);

    /** Read the data from the bucket.
     * @param b The bucket to read from
     * @param str A place to store the data read.  Allocation should only be
     *            done if absolutely necessary. 
     * @param len The amount of data read.
     * @param block Should this read function block if there is more data that
     *              cannot be read immediately.
     * @deffunc apr_status_t read(ap_bucket *b, const char **str, apr_size_t *len, ap_read_type block)
     */
    apr_status_t (*read)(ap_bucket *b, const char **str, apr_size_t *len, ap_read_type block);
    
    /** Make it possible to set aside the data. For most bucket types this is
     *  a no-op; buckets containing data that dies when the stack is un-wound
     *  must convert the bucket into a heap bucket.
     * @param e The bucket to convert
     * @deffunc apr_status_t setaside(ap_bucket *e)
     */
    apr_status_t (*setaside)(ap_bucket *e);

    /** Split one bucket in two at the specified position
     * @param e The bucket to split
     * @param point The offset of the first byte in the new bucket
     * @deffunc apr_status_t split(ap_bucket *e, apr_off_t point)
     */
    apr_status_t (*split)(ap_bucket *e, apr_off_t point);
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
#define AP_BRIGADE_SENTINEL(b)	AP_RING_SENTINEL(&(b)->list, ap_bucket, link)

#define AP_BRIGADE_EMPTY(b)	AP_RING_EMPTY(&(b)->list, ap_bucket, link)

#define AP_BRIGADE_FIRST(b)	AP_RING_FIRST(&(b)->list)
#define AP_BRIGADE_LAST(b)	AP_RING_LAST(&(b)->list)

#define AP_BRIGADE_FOREACH(e, b)					\
	AP_RING_FOREACH((e), &(b)->list, ap_bucket, link)

#define AP_BRIGADE_INSERT_HEAD(b, e)					\
	AP_RING_INSERT_HEAD(&(b)->list, (e), ap_bucket, link)
#define AP_BRIGADE_INSERT_TAIL(b, e)					\
	AP_RING_INSERT_TAIL(&(b)->list, (e), ap_bucket, link)

#define AP_BRIGADE_CONCAT(a, b)						\
	AP_RING_CONCAT(&(a)->list, &(b)->list, ap_bucket, link)

#define AP_BUCKET_INSERT_BEFORE(a, b)					\
	AP_RING_INSERT_BEFORE((a), (b), link)
#define AP_BUCKET_INSERT_AFTER(a, b)					\
	AP_RING_INSERT_AFTER((a), (b), link)

#define AP_BUCKET_NEXT(e)	AP_RING_NEXT((e), link)
#define AP_BUCKET_PREV(e)	AP_RING_PREV((e), link)

#define AP_BUCKET_REMOVE(e)	AP_RING_REMOVE((e), link)

#define AP_BUCKET_IS_FLUSH(e)       (e->type == &ap_flush_type)
#define AP_BUCKET_IS_EOS(e)         (e->type == &ap_eos_type)
#define AP_BUCKET_IS_FILE(e)        (e->type == &ap_file_type)
#define AP_BUCKET_IS_PIPE(e)        (e->type == &ap_pipe_type)
#define AP_BUCKET_IS_SOCKET(e)      (e->type == &ap_socket_type)
#define AP_BUCKET_IS_HEAP(e)        (e->type == &ap_heap_type)
#define AP_BUCKET_IS_TRANSIENT(e)   (e->type == &ap_transient_type)
#define AP_BUCKET_IS_IMMORTAL(e)    (e->type == &ap_immortal_type)
#define AP_BUCKET_IS_MMAP(e)        (e->type == &ap_mmap_type)
#define AP_BUCKET_IS_POOL(e)        (e->type == &ap_pool_type)

/**
 * General-purpose reference counting for the varous bucket types.
 *
 * Any bucket type that keeps track of the resources it uses (i.e.
 * most of them except for IMMORTAL, TRANSIENT, and EOS) needs to
 * attach a reference count to the resource so that it can be freed
 * when the last bucket that uses it goes away. Resource-sharing may
 * occur because of bucket splits or buckets that refer to globally
 * cached data. */

/**
 * The structure used to manage the shared resource must start with an
 * ap_bucket_refcount which is updated by the general-purpose refcount
 * code. A pointer to the bucket-type-dependent private data structure
 * can be cast to a pointer to an ap_bucket_refcount and vice versa.
 */
typedef struct ap_bucket_refcount ap_bucket_refcount;
struct ap_bucket_refcount {
    int          refcount;
};

/**
 * The data pointer of a refcounted bucket points to an
 * ap_bucket_shared structure which describes the region of the shared
 * object that this bucket refers to. The ap_bucket_shared isn't a
 * fully-fledged bucket type: it is a utility type that proper bucket
 * types are based on.
 */
typedef struct ap_bucket_shared ap_bucket_shared;
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
AP_DECLARE(ap_bucket_brigade *) ap_brigade_create(apr_pool_t *p);

/**
 * destroy an entire bucket brigade.  This includes destroying all of the
 * buckets within the bucket brigade's bucket list. 
 * @param b The bucket brigade to destroy
 * @deffunc apr_status_t ap_brigade_destroy(ap_bucket_brigade *b)
 */
AP_DECLARE(apr_status_t) ap_brigade_destroy(ap_bucket_brigade *b);

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
AP_DECLARE(ap_bucket_brigade *) ap_brigade_split(ap_bucket_brigade *b,
						 ap_bucket *e);

/**
 * consume nbytes from beginning of b -- call ap_bucket_destroy as
 * appropriate, and/or modify start on last element 
 * @param b The brigade to consume data from
 * @param nbytes The number of bytes to consume
 * @deffunc void ap_brigade_consume(ap_bucket_brigade *b, int nbytes) */
AP_DECLARE(void) ap_brigade_consume(ap_bucket_brigade *b, int nbytes);

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
AP_DECLARE(int) ap_brigade_to_iovec(ap_bucket_brigade *b, 
				    struct iovec *vec, int nvec);

/**
 * This function writes a list of strings into a bucket brigade.  We just 
 * allocate a new heap bucket for each string.
 * @param b The bucket brigade to add to
 * @param va A list of strings to add
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va)
 */
AP_DECLARE(int) ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va);

/**
 * This function writes an unspecified number of strings into a bucket brigade.
 * We just allocate a new heap bucket for each string.
 * @param b The bucket brigade to add to
 * @param ... The strings to add
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_putstrs(ap_bucket_brigade *b, ...)
 */
AP_DECLARE_NONSTD(int) ap_brigade_putstrs(ap_bucket_brigade *b, ...);

/**
 * Evaluate a printf and put the resulting string into a bucket at the end 
 * of the bucket brigade.
 * @param b The brigade to write to
 * @param fmt The format of the string to write
 * @param ... The arguments to fill out the format
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...) 
 */
AP_DECLARE_NONSTD(int) ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...);

/**
 * Evaluate a printf and put the resulting string into a bucket at the end 
 * of the bucket brigade.
 * @param b The brigade to write to
 * @param fmt The format of the string to write
 * @param va The arguments to fill out the format
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va) 
 */
AP_DECLARE(int) ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va);


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
 * @param point The location to split the bucket at
 * @deffunc apr_status_t ap_bucket_split(ap_bucket *e, apr_off_t point)
 */
#define ap_bucket_split(e,point) e->type->split(e, point)


/* Bucket type handling */

AP_DECLARE_NONSTD(apr_status_t) ap_bucket_setaside_notimpl(ap_bucket *data);
AP_DECLARE_NONSTD(apr_status_t) ap_bucket_split_notimpl(ap_bucket *data, 
                                                 apr_off_t point);
AP_DECLARE_NONSTD(void) ap_bucket_destroy_notimpl(void *data);
/* There is no ap_bucket_read_notimpl, because it is a required function
 */
int ap_insert_bucket_type(const ap_bucket_type *type);

/* All of the bucket types implemented by the core */
extern const ap_bucket_type ap_flush_type;
extern const ap_bucket_type ap_eos_type;
extern const ap_bucket_type ap_file_type;
extern const ap_bucket_type ap_heap_type;
extern const ap_bucket_type ap_mmap_type;
extern const ap_bucket_type ap_pool_type;
extern const ap_bucket_type ap_pipe_type;
extern const ap_bucket_type ap_immortal_type;
extern const ap_bucket_type ap_transient_type;
extern const ap_bucket_type ap_socket_type;


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
 * @deffunc AP_DECLARE(ap_bucket *) ap_bucket_shared_create(ap_bucket_refcount *r, apr_off_t start, apr_off_t end) */
AP_DECLARE(ap_bucket *) ap_bucket_make_shared(ap_bucket *b, void *data,
					      apr_off_t start, apr_off_t end);

/**
 * Decrement the refcount of the data in the bucket and free the
 * ap_bucket_shared structure. This function should only be called by
 * type-specific bucket destruction functions.
 * @param data The private data pointer from the bucket to be destroyed
 * @return NULL if nothing needs to be done,
 *         otherwise a pointer to the private data structure which
 *         must be destroyed because its reference count is zero
 * @deffunc AP_DECLARE(void *) ap_bucket_shared_destroy(ap_bucket *b) */
AP_DECLARE(void *) ap_bucket_destroy_shared(void *data);

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
 * @deffunc AP_DECLARE(apr_status_t) ap_bucket_shared_split(ap_bucket *b, apr_off_t point)
 */
AP_DECLARE_NONSTD(apr_status_t) ap_bucket_split_shared(ap_bucket *b, apr_off_t point);


/*  *****  Functions to Create Buckets of varying type  *****  */

/**
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
AP_DECLARE(ap_bucket *) ap_bucket_create_eos(void);
AP_DECLARE(ap_bucket *) ap_bucket_make_eos(ap_bucket *b);

/**
 * Create a flush  bucket.  This indicates that filters should flush their
 * data.  There is no guarantee that they will flush it, but this is the
 * best we can do.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_flush(void)
 */
AP_DECLARE(ap_bucket *) ap_bucket_create_flush(void);
AP_DECLARE(ap_bucket *) ap_bucket_make_flush(ap_bucket *b);

/**
 * Create a bucket referring to long-lived data.
 * @param buf The data to insert into the bucket
 * @param nbyte The size of the data to insert.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_transient(const char *buf, apr_size_t nbyte, apr_size_t *w)
 */
AP_DECLARE(ap_bucket *) ap_bucket_create_immortal(
		const char *buf, apr_size_t nbyte);
AP_DECLARE(ap_bucket *) ap_bucket_make_immortal(ap_bucket *b,
		const char *buf, apr_size_t nbyte);

/**
 * Create a bucket referring to data on the stack.
 * @param buf The data to insert into the bucket
 * @param nbyte The size of the data to insert.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_transient(const char *buf, apr_size_t nbyte, apr_size_t *w)
 */
AP_DECLARE(ap_bucket *) ap_bucket_create_transient(
		const char *buf, apr_size_t nbyte);
AP_DECLARE(ap_bucket *) ap_bucket_make_transient(ap_bucket *b,
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
AP_DECLARE(ap_bucket *) ap_bucket_create_heap(
		const char *buf, apr_size_t nbyte, int copy, apr_size_t *w);
AP_DECLARE(ap_bucket *) ap_bucket_make_heap(ap_bucket *b,
		const char *buf, apr_size_t nbyte, int copy, apr_size_t *w);

/**
 * Create a bucket referring to memory allocated out of a pool.
 * @param buf The buffer to insert into the bucket
 * @param p The pool the memory was allocated out of
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_pool(const char *buf, apr_size_t *length, apr_pool_t *p)
 */
AP_DECLARE(ap_bucket *) ap_bucket_create_pool(const char *buf,  
                                            apr_size_t length, apr_pool_t *p);
AP_DECLARE(ap_bucket *) ap_bucket_make_pool(ap_bucket *b,
		const char *buf, apr_size_t length, apr_pool_t *p);

/**
 * Create a bucket referring to mmap()ed memory.
 * @param mmap The mmap to insert into the bucket
 * @param start The offset of the first byte in the mmap
 *              that this bucket refers to
 * @param length The number of bytes referred to by this bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_mmap(const apr_mmap_t *buf, apr_size_t nbyte, apr_size_t *w)
 */
AP_DECLARE(ap_bucket *) ap_bucket_create_mmap(
		apr_mmap_t *mm, apr_off_t start, apr_size_t length);
AP_DECLARE(ap_bucket *) ap_bucket_make_mmap(ap_bucket *b,
		apr_mmap_t *mm, apr_off_t start, apr_size_t length);

/**
 * Create a bucket referring to a socket.
 * @param thissocket The socket to put in the bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_socket(apr_socket_t *thissocket)
 */
AP_DECLARE(ap_bucket *) ap_bucket_create_socket(apr_socket_t *thissock);
AP_DECLARE(ap_bucket *) ap_bucket_make_socket(ap_bucket *b, apr_socket_t *thissock);

/**
 * Create a bucket referring to a pipe.
 * @param thispipe The pipe to put in the bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_pipe(apr_file_t *thispipe)
 */
AP_DECLARE(ap_bucket *) ap_bucket_create_pipe(apr_file_t *thispipe);
AP_DECLARE(ap_bucket *) ap_bucket_make_pipe(ap_bucket *b, apr_file_t *thispipe);

/**
 * Create a bucket referring to a file.
 * @param fd The file to put in the bucket
 * @param offset The offset where the data of interest begins in the file
 * @param len The amount of data in the file we are interested in
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_file(apr_file_t *thispipe)
 */
AP_DECLARE(ap_bucket *) ap_bucket_create_file(apr_file_t *fd, apr_off_t offset, apr_size_t len);
AP_DECLARE(ap_bucket *) ap_bucket_make_file(ap_bucket *b, apr_file_t *fd, 
                                            apr_off_t offset, apr_size_t len);

#endif /* !AP_BUCKETS_H */
