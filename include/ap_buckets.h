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
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>	/* for struct iovec */
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

/**
 * @package Bucket Brigades
 */

typedef struct ap_bucket_brigade ap_bucket_brigade;

/* The basic concept behind bucket brigades.....
 *
 * A bucket brigade is simply a doubly linked list of buckets, so
 * we aren't limited to inserting at the front and removing at the
 * end.
 *
 * Buckets are just data stores.  They can be files, mmap areas, or just
 * pre-allocated memory.  The point of buckets is to store data.  Along with
 * that data, come some functions to access it.  The functions are relatively
 * simple, read, split, setaside, and destroy.
 *
 * read reads a string of data.  Currently, it assumes we read all of the 
 * data in the bucket.  This should be changed to only read the specified 
 * amount.
 *
 * split makes one bucket into two at the specified location.
 *
 * setaside ensures that the data in the bucket has a long enough lifetime.
 *
 * free destroys the data associated with the bucket.
 *
 * To write a bucket brigade, they are first made into an iovec, so that we
 * don't write too little data at one time.  Currently we ignore compacting the
 * buckets into as few buckets as possible, but if we really want good
 * performance, then we need to compact the buckets before we convert to an
 * iovec, or possibly while we are converting to an iovec.
 */

/* The types of bucket brigades the code knows about.  We don't really need
 * this enum.  All access to the bucket brigades is done through function
 * pointers in the bucket type.  However, when we start to do conversion
 * routines, this enum will be a huge performance benefit, so we leave it
 * alone.
 */
typedef enum {
    AP_BUCKET_HEAP,
    AP_BUCKET_TRANSIENT,
    AP_BUCKET_FILE,
    AP_BUCKET_MMAP,
    AP_BUCKET_IMMORTAL,
    AP_BUCKET_POOL,
    AP_BUCKET_PIPE,
    AP_BUCKET_EOS        /* End-of-stream bucket.  Special case to say this is
                          * the end of the brigade so all data should be sent
                          * immediately.
			  */
} ap_bucket_type_e;

#define AP_END_OF_BRIGADE       -1

typedef struct ap_bucket ap_bucket;

/**
 * ap_bucket_t structures are allocated on the malloc() heap and
 * their lifetime is controlled by the parent ap_brigade_t
 * structure. Buckets can move from one brigade to another e.g. by
 * calling ap_brigade_concat(). In general the data in a bucket has
 * the same lifetime as the bucket and is freed when the bucket is
 * destroyed; if the data is shared by more than one bucket (e.g.
 * after a split) the data is freed when the last bucket goes away.
 */
struct ap_bucket {
    /** The type of bucket.  These types can be found in the enumerated
     *  type above */
    ap_bucket_type_e type;
    /** type-dependent data hangs off this pointer */
    void *data;	
    /** The next bucket in the brigade */
    ap_bucket *next;
    /** The previous bucket in the brigade */
    ap_bucket *prev;
    /** The length of the data in the bucket.  This could have been implemented
     *  with a function, but this is an optimization, because the most
     *  common thing to do will be to get the length.  If the length is unknown,
     *  the value of this field will be -1.
     */
    apr_off_t length;

    /** @tip all of these function pointers may be replaced by some
     *  other means for getting to the functions, like a an index into
     *  a table.  In any case, these functions will always be available.
     */

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
     * @param len The amount of data read,
     *            or -1 (AP_END_OF_BRIGADE) if there is no more data
     * @param block Should this read function block if there is more data that
     *              cannot be read immediately.
     * @deffunc apr_status_t read(ap_bucket *b, const char **str, apr_ssize_t *len, int block)
     */
    apr_status_t (*read)(ap_bucket *b, const char **str, apr_ssize_t *len, int block);
    
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

/** A list of buckets */
struct ap_bucket_brigade {
    /** The pool to associate the brigade with.  The data is not allocated out
     *  of the pool, but a cleanup is registered with this pool.  If the 
     *  brigade is destroyed by some mechanism other than pool destruction,
     *  the destroying function is responsible for killing the cleanup.
     */
    apr_pool_t *p;
    /** The start of the bucket list. */
    ap_bucket *head;
    /** The end of the bucket list. */
    ap_bucket *tail;
};

/**
 * General-purpose reference counting for the varous bucket types.
 *
 * Any bucket type that keeps track of the resources it uses (i.e.
 * most of them except for IMMORTAL, TRANSIENT, and EOS) needs to
 * attach a reference count to the resource so that it can be freed
 * when the last bucket that uses it goes away. Resource-sharing may
 * occur because of bucket splits or buckets that refer to globally
 * cached data.
 */

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

/*  *****  Pipe buckets  *****  */


typedef struct ap_bucket_pipe ap_bucket_pipe;

/**
 * A bucket referring to a pipe.
 */
struct ap_bucket_pipe {
    apr_file_t *thepipe;
};

/*  *****  Bucket Brigade Functions  *****  */

/**
 * Create a new bucket brigade.  The bucket brigade is originally empty.
 * @param The pool to associate with the brigade.  Data is not allocated out
q *        of the pool, but a cleanup is registered.
 * @return The empty bucket brigade
 * @deffunc ap_bucket_brigade *ap_brigade_create(apr_pool_t *p)
 */
API_EXPORT(ap_bucket_brigade *) ap_brigade_create(apr_pool_t *p);

/**
 * destroy an enitre bucket brigade.  This includes destroying all of the
 * buckets within the bucket brigade's bucket list. 
 * @param b The bucket brigade to destroy
 * @deffunc apr_status_t ap_brigade_destroy(ap_bucket_brigade *b)
 */
API_EXPORT(apr_status_t) ap_brigade_destroy(ap_bucket_brigade *b);

/**
 * append bucket(s) to a bucket_brigade.  This is the correct way to add
 * buckets to the end of a bucket briagdes bucket list.  This will accept
 * a list of buckets of any length.
 * @param b The bucket brigade to append to
 * @param e The bucket list to append
 * @deffunc void ap_brigade_append_buckets(ap_bucket_brigade *b, ap_bucket *e)
 */
API_EXPORT(void) ap_brigade_append_buckets(ap_bucket_brigade *b,
                                                  ap_bucket *e);

/**
 * consume nbytes from beginning of b -- call ap_bucket_destroy as
 * appropriate, and/or modify start on last element 
 * @param b The brigade to consume data from
 * @param nbytes The number of bytes to consume
 * @deffunc void ap_brigade_consume(ap_bucket_brigade *b, int nbytes)
 */
API_EXPORT(void) ap_brigade_consume(ap_bucket_brigade *b, int nbytes);

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
API_EXPORT(int) ap_brigade_to_iovec(ap_bucket_brigade *b, 
                                           struct iovec *vec, int nvec);

/**
 * Concatenate bucket_brigade b onto the end of bucket_brigade a,
 * emptying bucket_brigade b in the process. Neither bucket brigade
 * can be NULL, but either one of them can be emtpy when calling this
 * function.
 * @param a The brigade to catenate to.
 * @param b The brigade to add to a.  This brigade will be empty on return
 * @deffunc void ap_brigade_catenate(ap_bucket_brigade *a, ap_bucket_brigade *b)
 */
API_EXPORT(void) ap_brigade_catenate(ap_bucket_brigade *a, 
                                            ap_bucket_brigade *b);

/**
 * This function writes a list of strings into a bucket brigade.  We just 
 * allocate a new heap bucket for each string.
 * @param b The bucket brigade to add to
 * @param va A list of strings to add
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va)
 */
API_EXPORT(int) ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va);

/**
 * Evaaluate a printf and put the resulting string into a bucket at the end 
 * of the bucket brigade.
 * @param b The brigade to write to
 * @param fmt The format of the string to write
 * @param ... The arguments to fill out the format
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...) 
 */
API_EXPORT(int) ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...);

/**
 * Evaluate a printf and put the resulting string into a bucket at the end 
 * of the bucket brigade.
 * @param b The brigade to write to
 * @param fmt The format of the string to write
 * @param va The arguments to fill out the format
 * @return The number of bytes added to the brigade
 * @deffunc int ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va) 
 */
API_EXPORT(int) ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va);


/*  *****  Bucket Functions  *****  */

/**
 * free the resources used by a bucket. If multiple buckets refer to
 * the same resource it is freed when the last one goes away.
 * @param e The bucket to destroy
 * @deffunc apr_status_t ap_bucket_destroy(ap_bucket *e)
 */
API_EXPORT(apr_status_t) ap_bucket_destroy(ap_bucket *e);


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
 * @deffunc API_EXPORT(ap_bucket *) ap_bucket_shared_create(ap_bucket_refcount *r, apr_off_t start, apr_off_t end) */
API_EXPORT(ap_bucket *) ap_bucket_make_shared(ap_bucket *b, void *data,
					      apr_off_t start, apr_off_t end);

/**
 * Decrement the refcount of the data in the bucket and free the
 * ap_bucket_shared structure. This function should only be called by
 * type-specific bucket destruction functions.
 * @param data The private data pointer from the bucket to be destroyed
 * @return NULL if nothing needs to be done,
 *         otherwise a pointer to the private data structure which
 *         must be destroyed because its reference count is zero
 * @deffunc API_EXPORT(void *) ap_bucket_shared_destroy(ap_bucket *b) */
API_EXPORT(void *) ap_bucket_destroy_shared(void *data);

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
 * @deffunc API_EXPORT(apr_status_t) ap_bucket_shared_split(ap_bucket *b, apr_off_t point)
 */
API_EXPORT_NONSTD(apr_status_t) ap_bucket_split_shared(ap_bucket *b, apr_off_t point);


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
	ap__b->next = NULL;			\
	ap__b->prev = NULL;			\
	return ap__b;				\
    } while(0)


/**
 * Create an End of Stream bucket.  This indicates that there is no more data
 * coming from down the filter stack
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_eos(void)
 */
API_EXPORT(ap_bucket *) ap_bucket_create_eos(void);
API_EXPORT(ap_bucket *) ap_bucket_make_eos(ap_bucket *b);


/**
 * Create a bucket referring to long-lived data.
 * @param buf The data to insert into the bucket
 * @param nbyte The size of the data to insert.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_transient(const char *buf, apr_size_t nbyte, apr_ssize_t *w)
 */
API_EXPORT(ap_bucket *) ap_bucket_create_immortal(
		const char *buf, apr_size_t nbyte);
API_EXPORT(ap_bucket *) ap_bucket_make_immortal(ap_bucket *b,
		const char *buf, apr_size_t nbyte);

/**
 * Create a bucket referring to data on the stack.
 * @param buf The data to insert into the bucket
 * @param nbyte The size of the data to insert.
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_transient(const char *buf, apr_size_t nbyte, apr_ssize_t *w)
 */
API_EXPORT(ap_bucket *) ap_bucket_create_transient(
		const char *buf, apr_size_t nbyte);
API_EXPORT(ap_bucket *) ap_bucket_make_transient(ap_bucket *b,
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
 * @deffunc ap_bucket *ap_bucket_create_heap(const char *buf, apr_size_t nbyte, apr_ssize_t *w)
 */
API_EXPORT(ap_bucket *) ap_bucket_create_heap(
		const char *buf, apr_size_t nbyte, int copy, apr_ssize_t *w);
API_EXPORT(ap_bucket *) ap_bucket_make_heap(ap_bucket *b,
		const char *buf, apr_size_t nbyte, int copy, apr_ssize_t *w);

/**
 * Create a bucket referring to mmap()ed memory.
 * @param mmap The mmap to insert into the bucket
 * @param start The offset of the first byte in the mmap
 *              that this bucket refers to
 * @param length The number of bytes referred to by this bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_mmap(const apr_mmap_t *buf, apr_size_t nbyte, apr_ssize_t *w)
 */
API_EXPORT(ap_bucket *) ap_bucket_create_mmap(
		apr_mmap_t *mm, apr_off_t start, apr_size_t length);
API_EXPORT(ap_bucket *) ap_bucket_make_mmap(ap_bucket *b,
		apr_mmap_t *mm, apr_off_t start, apr_size_t length);

/**
 * Create a bucket referring to a pipe.
 * @param thispipe The pipe to put in the bucket
 * @return The new bucket, or NULL if allocation failed
 * @deffunc ap_bucket *ap_bucket_create_pipe(apr_file_t *thispipe)
 */
API_EXPORT(ap_bucket *) ap_bucket_create_pipe(apr_file_t *thispipe);
API_EXPORT(ap_bucket *) ap_bucket_make_pipe(ap_bucket *b, apr_file_t *thispipe);

#endif /* !AP_BUCKETS_H */
