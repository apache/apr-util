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

#ifndef AP_BUF_H
#define AP_BUF_H

#include "httpd.h"
#include "apr_general.h"
#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_private.h"
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
 * A bucket brigade is simply a doubly linked list of buckets, where
 * we aren't limited to inserting at the front and removing at the
 * end.
 *
 * Buckets are just data stores.  They can be files, mmap areas, or just
 * pre-allocated memory.  The point of buckets is to store data.  Along with
 * that data, come some functions to access it.  The functions are relatively
 * simple, read, write, getlen, split, and free.
 *
 * read reads a string of data.  Currently, it assumes we read all of the 
 * data in the bucket.  This should be changed to only read the specified 
 * amount.
 *
 * getlen gets the number of bytes stored in the bucket.
 * 
 * write writes the specified data to the bucket.  Depending on the type of
 * bucket, this may append to the end of previous data, or wipe out the data
 * currently in the bucket.  heap buckets append currently, all others 
 * erase the current bucket.
 *
 * split just makes one bucket into two at the specified location.  To implement
 * this correctly, we really need to implement reference counting.
 *
 * free just destroys the data associated with the bucket.
 *
 * We may add more functions later.  There has been talk of needing a stat,
 * which would probably replace the getlen.  And, we definately need a convert
 * function.  Convert would make one bucket type into another bucket type.
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
     *  common thing to do will be to get the length.
     */
    apr_ssize_t length;

    /** @tip all of these function pointers may be replaced by some
     *  other means for getting to the functions, like a an index into
     *  a table.  In any case, these functions will always be available.
     */

    /** A function pointer to destroy the data in the bucket and the bucket
     *  itself.  Some of the buckets implement reference couting, and those
     * buckets all have a sub-bucket which is not destroyed until the
     * reference count is zero. */
    void (*destroy)(ap_bucket *e);                /* can be NULL */

    /** Read the data from the bucket.
     * @param b The bucket to read from
     * @param str A place to store the data read.  Allocation should only be
     *            done if absolutely necessary. 
     * @param len The amount of data read.
     * @param block Should this read function block if there is more data that
     *              cannot be read immediately.
     * @deffunc apr_status_t read(ap_bucket *b, const char **str, apr_ssize_t *len, int block)
     */
    apr_status_t (*read)(ap_bucket *b, const char **str, apr_ssize_t *len, int block);
    
    /** Make it possible to set aside the data. For most bucket types this is
     *  a no-op; buckets containing data that dies when the stack is un-wound
     *  must convert the bucket into a heap bucket.
     * @param e The bucket to convert
     * @deffunc void setaside(ap_bucket *e)
     */
    void (*setaside)(ap_bucket *e);

    /** Split one bucket in two at the specified position
     * @param e The bucket to split
     * @param nbytes The offset at which to split the bucket
     * @deffunc apr_status_t split(ap_bucket *e, apr_off_t nbytes)
     */
    apr_status_t (*split)(ap_bucket *e, apr_off_t nbytes);
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

/*    ******  Different bucket types   *****/

typedef struct ap_bucket_transient ap_bucket_transient;

/**
 * A bucket containing data on the stack that will be destroyed as the
 * stack is unwound.
 */
struct ap_bucket_transient {
    /** The start of the data in the bucket */
    const char    *start;
    /** The end of the data in the bucket */
    const char    *end;
};

typedef struct ap_bucket_heap ap_bucket_heap;

/**
 * A bucket containing data allocated off the heap.
 */
struct ap_bucket_heap {
    /** The start of the data actually allocated.  This should never be
     * modified, it is only used to free the bucket.
     */
    char    *alloc_addr;
    /** how much memory was allocated.  This may not be necessary */
    size_t  alloc_len;
    /** Where does the data the bucket is actually referencing start */
    char    *start;
    /** Where does the data the bucket cares about end */               
    char    *end;
};

typedef struct ap_bucket_mmap ap_bucket_mmap;
typedef struct ap_mmap_sub_bucket ap_mmap_sub_bucket;

/**
 * The sub mmap bucket.  This is the meat of the reference count implementation
 * mmaps aren't actually un-mapped until the ref count is zero.
 */
struct ap_mmap_sub_bucket {
    /** The mmap this sub_bucket refers to */
    const apr_mmap_t *mmap;
    /** The current reference count for this sub_bucket */
    int refcount;
};

/** A bucket representing an mmap object */
struct ap_bucket_mmap {
    /** Where does this buckets section of the mmap start */
    char      *start;
    /** Where does this buckets section of the mmap end */
    char      *end;
    /** The mmap sub_bucket referenced by this bucket */
    ap_mmap_sub_bucket *sub;  /* The mmap and ref count */    
};

/*   ******  Bucket Brigade Functions  *****  */

/**
 * Create a new bucket brigade.  The bucket brigade is originally empty.
 * @param The pool to associate with the brigade.  Data is not allocated out
 *        of the pool, but a cleanup is registered.
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

/*   ******  Bucket Functions  *****  */

/**
 * free the resources used by a bucket. If multiple buckets refer to
 * the same resource it is freed when the last one goes away.
 * @param e The bucket to destroy
 * @deffunc apr_status_t ap_bucket_destroy(ap_bucket *e)
 */
API_EXPORT(apr_status_t) ap_bucket_destroy(ap_bucket *e);

/****** Functions to Create Buckets of varying type ******/

/*
 * All of these functions are responsibly for creating a bucket and filling
 * it out with an initial value.  Some buckets can be over-written, others
 * can't.  What should happen is that buckets that can't be over-written,
 * will have NULL write functions.  That is currently broken, although it is
 * easy to fix.  The creation routines may not allocate the space for the
 * buckets, because we may be using a free list.  Regardless, creation
 * routines are responsible for getting space for a bucket from someplace
 * and inserting the initial data.
 */

/**
 * Create a bucket referring to memory on the heap. This always
 * allocates 4K of memory, so that the bucket can grow without
 * requiring another allocation.
 * @param buf The buffer to insert into the bucket
 * @param nbyte The size of the buffer to insert.
 * @param w The number of bytes actually inserted into the bucket
 * @return The new bucket
 * @deffunc ap_bucket *ap_bucket_heap_create(const char *buf, apr_size_t nbyte, apr_ssize_t *w)
 */
API_EXPORT(ap_bucket *) ap_bucket_heap_create(const char *buf,
                                apr_size_t nbyte, apr_ssize_t *w);


/**
 * Create a mmap memory bucket, and initialize the ref count to 1
 * @param buf The mmap to insert into the bucket
 * @param nbyte The size of the mmap to insert.
 * @param w The number of bytes actually inserted into the bucket
 * @return The new bucket
 * @deffunc ap_bucket *ap_bucket_mmap_create(const apr_mmap_t *buf, apr_size_t nbyte, apr_ssize_t *w)
 */
API_EXPORT(ap_bucket *) ap_bucket_mmap_create(const apr_mmap_t *buf,
                                      apr_size_t nbytes, apr_ssize_t *w);

/**
 * Create a transient memory bucket.
 * @param buf The data to insert into the bucket
 * @param nbyte The size of the data to insert.
 * @param w The number of bytes actually inserted into the bucket
 * @return The new bucket
 * @deffunc ap_bucket *ap_bucket_transient_create(const char *buf, apr_size_t nbyte, apr_ssize_t *w)
 */
API_EXPORT(ap_bucket *) ap_bucket_transient_create(const char *buf,
                               apr_size_t nbyte, apr_ssize_t *w);

/**
 * Create an End of Stream bucket.  This indicates that there is no more data
 * coming from down the filter stack
 * @return The new bucket
 * @deffunc ap_bucket *ap_bucket_eos_create(void)
 */
API_EXPORT(ap_bucket *) ap_bucket_eos_create(void);

#endif

