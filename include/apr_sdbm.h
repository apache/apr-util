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

/*
 * sdbm - ndbm work-alike hashed database library
 * based on Per-Ake Larson's Dynamic Hashing algorithms. BIT 18 (1978).
 * author: oz@nexus.yorku.ca
 * status: ex-public domain
 */

#ifndef SDBM_H
#define SDBM_H

#include "apr_errno.h"
#include "apr_file_io.h"   /* for apr_fileperms_t */

typedef struct SDBM SDBM;

/* utility functions */
int sdbm_rdonly(SDBM *db);
int sdbm_error(SDBM *db);
int sdbm_clearerr(SDBM *db);

typedef struct {
    char *dptr;
    int dsize;
} sdbm_datum;

/* The extensions used for the database files */
#define SDBM_DIRFEXT	".dir"
#define SDBM_PAGFEXT	".pag"

/* Standard dbm interface */

apr_status_t sdbm_open(SDBM **db, const char *filename, apr_int32_t flags,
                       apr_fileperms_t perms, apr_pool_t *p);

void sdbm_close(SDBM *db); /* ### should return value? */

sdbm_datum sdbm_fetch(SDBM *db, sdbm_datum key);
apr_status_t sdbm_delete(SDBM *db, const sdbm_datum key);

/* * flags to sdbm_store */
#define SDBM_INSERT	0
#define SDBM_REPLACE	1
apr_status_t sdbm_store(SDBM *db, sdbm_datum key, sdbm_datum value, int flags);
sdbm_datum sdbm_firstkey(SDBM *db);
sdbm_datum sdbm_nextkey(SDBM *db);

/*
 * other
 */
apr_status_t sdbm_prep(SDBM **db, const char *dirname, const char *pagname,
                       apr_int32_t flags, apr_fileperms_t perms,
                       apr_pool_t *pool);

long sdbm_hash(const char *str, int len);

#endif /* SDBM_H */
