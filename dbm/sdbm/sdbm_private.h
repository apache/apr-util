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

/*
 * sdbm - ndbm work-alike hashed database library
 * based on Per-Ake Larson's Dynamic Hashing algorithms. BIT 18 (1978).
 * author: oz@nexus.yorku.ca
 */

#ifndef SDBM_PRIVATE_H
#define SDBM_PRIVATE_H

#include "apr.h"
#include "apr_pools.h"
#include "apr_file_io.h"
#include "apr_errno.h" /* for apr_status_t */

#if 0
/* if the block/page size is increased, it breaks perl apr_sdbm_t compatibility */
#define DBLKSIZ 16384
#define PBLKSIZ 8192
#define PAIRMAX 8008			/* arbitrary on PBLKSIZ-N */
#else
#define DBLKSIZ 4096
#define PBLKSIZ 1024
#define PAIRMAX 1008			/* arbitrary on PBLKSIZ-N */
#endif
#define SPLTMAX	10			/* maximum allowed splits */

/* for apr_sdbm_t.flags */
#define SDBM_RDONLY	        0x1    /* data base open read-only */
#define SDBM_SHARED	        0x2    /* data base open for sharing */
#define SDBM_SHARED_LOCK	0x4    /* data base locked for shared read */
#define SDBM_EXCLUSIVE_LOCK	0x8    /* data base locked for write */

struct apr_sdbm_t {
    apr_pool_t *pool;
    apr_file_t *dirf;		       /* directory file descriptor */
    apr_file_t *pagf;		       /* page file descriptor */
    apr_int32_t flags;		       /* status/error flags, see below */
    long maxbno;		       /* size of dirfile in bits */
    long curbit;		       /* current bit number */
    long hmask;			       /* current hash mask */
    long blkptr;		       /* current block for nextkey */
    int  keyptr;		       /* current key for nextkey */
    long blkno;			       /* current page to read/write */
    long pagbno;		       /* current page in pagbuf */
    char pagbuf[PBLKSIZ];	       /* page file block buffer */
    long dirbno;		       /* current block in dirbuf */
    char dirbuf[DBLKSIZ];	       /* directory file block buffer */
    int  lckcnt;                       /* number of calls to sdbm_lock */
};

extern const apr_sdbm_datum_t sdbm_nullitem;

long sdbm_hash(const char *str, int len);

/*
 * zero the cache
 */
#define SDBM_INVALIDATE_CACHE(db, finfo) \
    do { db->dirbno = (!finfo.size) ? 0 : -1; \
         db->pagbno = -1; \
         db->maxbno = (long)(finfo.size * BYTESIZ); \
    } while (0);

#endif /* SDBM_PRIVATE_H */
