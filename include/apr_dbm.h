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

#ifndef APR_DBM_H
#define APR_DBM_H

#include "apu.h"
#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @package APR-UTIL DBM library
 */

/**
 * Structure for referencing a dbm
 * @defvar apr_dbm_t
 */
typedef struct apr_dbm_t apr_dbm_t;

/**
 * Structure for referencing the datum record within a dbm
 * @defvar apr_datum_t
 */
typedef struct
{
    char *dptr;
    apr_size_t dsize;
} apr_datum_t;

/* modes to open the DB */
#define APR_DBM_READONLY        1       /* open for read-only access */
#define APR_DBM_READWRITE       2       /* open for read-write access */
#define APR_DBM_RWCREATE        3       /* open for r/w, create if needed */

/**
 * Open a dbm file by file name
 * @param dbm The newly opened database
 * @param name The dbm file name to open
 * @param mode The flag value
 * <PRE>
 *           APR_DBM_READONLY   open for read-only access
 *           APR_DBM_READWRITE  open for read-write access
 *           APR_DBM_RWCREATE   open for r/w, create if needed
 * </PRE>
 * @param cntxt The pool to use when creating the dbm
 * @tip The dbm name may not be a true file name, as many dbm packages
 * append suffixes for seperate data and index files.
 */
apr_status_t apr_dbm_open(apr_dbm_t **dbm, const char *name, int mode,
                          apr_pool_t *cntxt);
/**
 * Close a dbm file previously opened by apr_dbm_open
 * @param dbm The database to close
 */
void apr_dbm_close(apr_dbm_t *db);
/**
 * Fetch a dbm record value by key
 * @param dbm The database 
 * @param key The key datum to find this record
 * @param value The value datum retrieved for this record
 */
apr_status_t apr_dbm_fetch(apr_dbm_t *dbm, apr_datum_t key,
                           apr_datum_t *pvalue);
/**
 * Store a dbm record value by key
 * @param dbm The database 
 * @param key The key datum to store this record by
 * @param value The value datum to store in this record
 */
apr_status_t apr_dbm_store(apr_dbm_t *dbm, apr_datum_t key, apr_datum_t value);
/**
 * Delete a dbm record value by key
 * @param dbm The database 
 * @param key The key datum of the record to delete
 */
apr_status_t apr_dbm_delete(apr_dbm_t *dbm, apr_datum_t key);
/**
 * Search for a key within the dbm
 * @param dbm The database 
 * @param key The datum describing a key to test
 */
int apr_dbm_exists(apr_dbm_t *dbm, apr_datum_t key);
/**
 * Retrieve the first record key from a dbm
 * @param dbm The database 
 * @param key The key datum of the first record
 */
apr_status_t apr_dbm_firstkey(apr_dbm_t *dbm, apr_datum_t *pkey);
/**
 * Retrieve the next record key from a dbm
 * @param dbm The database 
 * @param key The key datum of the next record
 */
apr_status_t apr_dbm_nextkey(apr_dbm_t *dbm, apr_datum_t *pkey);

/* XXX: This is bogus.  If this is a pool-managed dbm wrapper, we
 * don't free datum.  If it isn't why pass pools? 
 */
void apr_dbm_freedatum(apr_dbm_t *dbm, apr_datum_t data);

/* XXX: Need to change errcode to be handled canonically, and modify
 * the prototype to follow apr_dso_error etc.
 */
void apr_dbm_geterror(apr_dbm_t *dbm, int *errcode, const char **errmsg);

#ifdef __cplusplus
}
#endif

#endif	/* !APR_DBM_H */


