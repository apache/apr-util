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

#ifndef APR_DBM_PRIVATE_H
#define APR_DBM_PRIVATE_H

#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_dbm.h"
#include "apr_file_io.h"

#include "apu.h"

/* ### for now, include the DBM selection; this will go away once we start
   ### building and linking all of the DBMs at once. */
#include "apu_select_dbm.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @internal */

/**
 * Most DBM libraries take a POSIX mode for creating files.  Don't trust
 * the mode_t type, some platforms may not support it, int is safe.
 */
APU_DECLARE(int) apr_posix_perms2mode(apr_fileperms_t perm);

/**
 * Structure to describe the operations of the DBM
 */
typedef struct {
    /** The name of the DBM Type */
    const char *name;

    /** Open the DBM */
    apr_status_t (*open)(apr_dbm_t **pdb, const char *pathname,
                         apr_int32_t mode, apr_fileperms_t perm,
                         apr_pool_t *pool);

    /** Close the DBM */
    void (*close)(apr_dbm_t *dbm);

    /** Fetch a dbm record value by key */
    apr_status_t (*fetch)(apr_dbm_t *dbm, apr_datum_t key,
                                   apr_datum_t * pvalue);

    /** Store a dbm record value by key */
    apr_status_t (*store)(apr_dbm_t *dbm, apr_datum_t key, apr_datum_t value);

    /** Delete a dbm record value by key */
    apr_status_t (*del)(apr_dbm_t *dbm, apr_datum_t key);

    /** Search for a key within the dbm */
    int (*exists)(apr_dbm_t *dbm, apr_datum_t key);

    /** Retrieve the first record key from a dbm */
    apr_status_t (*firstkey)(apr_dbm_t *dbm, apr_datum_t * pkey);

    /** Retrieve the next record key from a dbm */
    apr_status_t (*nextkey)(apr_dbm_t *dbm, apr_datum_t * pkey);

    /** Proactively toss any memory associated with the apr_datum_t. */
    void (*freedatum)(apr_dbm_t *dbm, apr_datum_t data);

    /** Get the names that the DBM will use for a given pathname. */
    void (*getusednames)(apr_pool_t *pool,
                         const char *pathname,
                         const char **used1,
                         const char **used2);

} apr_dbm_type_t;


/**
 * The actual DBM
 */
struct apr_dbm_t
{ 
    /** Associated pool */
    apr_pool_t *pool;

    /** pointer to DB Implementation Specific data */
    void *file;

    /** Current integer error code */
    int errcode;
    /** Current string error code */
    const char *errmsg;

    /** the type of DBM */
    const apr_dbm_type_t *type;
};


/* Declare all of the builtin DBM providers */
APU_DECLARE_DATA extern const apr_dbm_type_t apr_dbm_type_sdbm;
APU_DECLARE_DATA extern const apr_dbm_type_t apr_dbm_type_gdbm;
APU_DECLARE_DATA extern const apr_dbm_type_t apr_dbm_type_ndbm;
APU_DECLARE_DATA extern const apr_dbm_type_t apr_dbm_type_db1;
APU_DECLARE_DATA extern const apr_dbm_type_t apr_dbm_type_db2;
APU_DECLARE_DATA extern const apr_dbm_type_t apr_dbm_type_db3;
APU_DECLARE_DATA extern const apr_dbm_type_t apr_dbm_type_db4;
APU_DECLARE_DATA extern const apr_dbm_type_t apr_dbm_type_db;

#ifdef __cplusplus
}
#endif

#endif /* APR_DBM_PRIVATE_H */
