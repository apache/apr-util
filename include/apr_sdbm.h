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
 * status: ex-public domain
 */

#ifndef APR_SDBM_H
#define APR_SDBM_H

#include "apu.h"
#include "apr_errno.h"
#include "apr_file_io.h"   /* for apr_fileperms_t */

/** 
 * @file apr_sdbm.h
 * @brief apr-util SDBM library
 */
/**
 * @defgroup APR_Util_DBM_SDBM SDBM library
 * @ingroup APR_Util_DBM
 * @{
 */

/**
 * Structure for referencing an sdbm
 */
typedef struct apr_sdbm_t apr_sdbm_t;

/**
 * Structure for referencing the datum record within an sdbm
 */
typedef struct {
    /** pointer to the data stored/retrieved */
    char *dptr;
    /** size of data */
    int dsize;
} apr_sdbm_datum_t;

/* The extensions used for the database files */
/** SDBM Directory file extension */
#define APR_SDBM_DIRFEXT	".dir"
/** SDBM page file extension */
#define APR_SDBM_PAGFEXT	".pag"

/* flags to sdbm_store */
#define APR_SDBM_INSERT     0   /**< Insert */
#define APR_SDBM_REPLACE    1   /**< Replace */
#define APR_SDBM_INSERTDUP  2   /**< Insert with duplicates */

/**
 * Open an sdbm database by file name
 * @param db The newly opened database
 * @param name The sdbm file to open
 * @param mode The flag values (APR_READ and APR_BINARY flags are implicit)
 * <PRE>
 *           APR_WRITE          open for read-write access
 *           APR_CREATE         create the sdbm if it does not exist
 *           APR_TRUNCATE       empty the contents of the sdbm
 *           APR_EXCL           fail for APR_CREATE if the file exists
 *           APR_DELONCLOSE     delete the sdbm when closed
 *           APR_SHARELOCK      support locking across process/machines
 * </PRE>
 * @param perms Permissions to apply to if created
 * @param p The pool to use when creating the sdbm
 * @remark The sdbm name is not a true file name, as sdbm appends suffixes 
 * for seperate data and index files.
 */
APU_DECLARE(apr_status_t) apr_sdbm_open(apr_sdbm_t **db, const char *name, 
                                        apr_int32_t mode, 
                                        apr_fileperms_t perms, apr_pool_t *p);

/**
 * Close an sdbm file previously opened by apr_sdbm_open
 * @param db The database to close
 */
APU_DECLARE(apr_status_t) apr_sdbm_close(apr_sdbm_t *db);

/**
 * Lock an sdbm database for concurency of multiple operations
 * @param db The database to lock
 * @param type The lock type
 * <PRE>
 *           APR_FLOCK_SHARED
 *           APR_FLOCK_EXCLUSIVE
 * </PRE>
 * @remark Calls to apr_sdbm_lock may be nested.  All apr_sdbm functions
 * perform implicit locking.  Since an APR_FLOCK_SHARED lock cannot be 
 * portably promoted to an APR_FLOCK_EXCLUSIVE lock, apr_sdbm_store and 
 * apr_sdbm_delete calls will fail if an APR_FLOCK_SHARED lock is held.
 * The apr_sdbm_lock call requires the database to be opened with the
 * APR_SHARELOCK mode value.
 */
APU_DECLARE(apr_status_t) apr_sdbm_lock(apr_sdbm_t *db, int type);

/**
 * Release an sdbm lock previously aquired by apr_sdbm_lock
 * @param db The database to unlock
 */
APU_DECLARE(apr_status_t) apr_sdbm_unlock(apr_sdbm_t *db);

/**
 * Fetch an sdbm record value by key
 * @param db The database 
 * @param value The value datum retrieved for this record
 * @param key The key datum to find this record
 */
APU_DECLARE(apr_status_t) apr_sdbm_fetch(apr_sdbm_t *db, 
                                         apr_sdbm_datum_t *value, 
                                         apr_sdbm_datum_t key);

/**
 * Store an sdbm record value by key
 * @param db The database 
 * @param key The key datum to store this record by
 * @param value The value datum to store in this record
 * @param opt The method used to store the record
 * <PRE>
 *           APR_SDBM_INSERT     return an error if the record exists
 *           APR_SDBM_REPLACE    overwrite any existing record for key
 * </PRE>
 */
APU_DECLARE(apr_status_t) apr_sdbm_store(apr_sdbm_t *db, apr_sdbm_datum_t key,
                                         apr_sdbm_datum_t value, int opt);

/**
 * Delete an sdbm record value by key
 * @param db The database 
 * @param key The key datum of the record to delete
 * @remark It is not an error to delete a non-existent record.
 */
APU_DECLARE(apr_status_t) apr_sdbm_delete(apr_sdbm_t *db, 
                                          const apr_sdbm_datum_t key);

/**
 * Retrieve the first record key from a dbm
 * @param db The database 
 * @param key The key datum of the first record
 * @remark The keys returned are not ordered.  To traverse the list of keys
 * for an sdbm opened with APR_SHARELOCK, the caller must use apr_sdbm_lock
 * prior to retrieving the first record, and hold the lock until after the
 * last call to apr_sdbm_nextkey.
 */
APU_DECLARE(apr_status_t) apr_sdbm_firstkey(apr_sdbm_t *db, apr_sdbm_datum_t *key);

/**
 * Retrieve the next record key from an sdbm
 * @param db The database 
 * @param key The key datum of the next record
 */
APU_DECLARE(apr_status_t) apr_sdbm_nextkey(apr_sdbm_t *db, apr_sdbm_datum_t *key);

/**
 * Returns true if the sdbm database opened for read-only access
 * @param db The database to test
 */
APU_DECLARE(int) apr_sdbm_rdonly(apr_sdbm_t *db);
/** @} */
#endif /* APR_SDBM_H */
