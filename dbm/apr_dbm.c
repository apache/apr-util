/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_strings.h"
#define APR_WANT_MEMFUNC
#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "apu_select_dbm.h"
#include "apr_dbm.h"


/* this is used in a few places to define a noop "function". it is needed
   to stop "no effect" warnings from GCC. */
#define NOOP_FUNCTION if (0) ; else

/* ### define defaults for now; these will go away in a while */
#define REGISTER_CLEANUP(dbm, pdatum) NOOP_FUNCTION
#define SET_FILE(pdb, f) ((pdb)->file = (f))


/* ### note: the setting of DBM_VTABLE will go away once we have multiple
   ### DBMs in here. */

#if APU_USE_SDBM

#include "apr_dbm_sdbm.c"
#define DBM_VTABLE apr_dbm_type_sdbm

#elif APU_USE_GDBM

#include "apr_dbm_gdbm.c"
#define DBM_VTABLE apr_dbm_type_gdbm

#elif APU_USE_DB

#include "apr_dbm_berkeleydb.c"
#define DBM_VTABLE apr_dbm_type_db

#else /* Not in the USE_xDBM list above */
#error a DBM implementation was not specified
#endif



APU_DECLARE(apr_status_t) apr_dbm_open(apr_dbm_t **pdb, const char *pathname, 
                                       apr_int32_t mode, apr_fileperms_t perm,
                                       apr_pool_t *pool)
{
    real_file_t file;
    int dbmode;

    *pdb = NULL;

    switch (mode) {
    case APR_DBM_READONLY:
        dbmode = APR_DBM_DBMODE_RO;
        break;
    case APR_DBM_READWRITE:
        dbmode = APR_DBM_DBMODE_RW;
        break;
    case APR_DBM_RWCREATE:
        dbmode = APR_DBM_DBMODE_RWCREATE;
        break;
    case APR_DBM_RWTRUNC:
        dbmode = APR_DBM_DBMODE_RWTRUNC;
        break;
    default:
        return APR_EINVAL;
    }

#if APU_USE_SDBM

    {
        apr_status_t rv;

        rv = apr_sdbm_open(&file, pathname, dbmode, perm, pool);
        if (rv != APR_SUCCESS)
            return rv;
    }

#elif APU_USE_GDBM

    {
        /* Note: stupid cast to get rid of "const" on the pathname */
        file = gdbm_open((char *) pathname, 0, dbmode,
                         apr_posix_perms2mode(perm), NULL);
        if (file == NULL)
            return APR_EGENERAL;      /* ### need a better error */
    }

#elif APU_USE_DB

    {
        int dberr;

#if DB_VER == 3
        if ((dberr = db_create(&file.bdb, NULL, 0)) == 0) {
            if ((dberr = (*file.bdb->open)(file.bdb, pathname, NULL, 
                                           DB_HASH, dbmode, 
                                           apr_posix_perms2mode(perm))) != 0) {
                /* close the DB handler */
                (void) (*file.bdb->close)(file.bdb, 0);
            }
        }
        file.curs = NULL;
#elif DB_VER == 2
        dberr = db_open(pathname, DB_HASH, dbmode, apr_posix_perms2mode(perm),
                        NULL, NULL, &file.bdb);
        file.curs = NULL;
#else
        file.bdb = dbopen(pathname, dbmode, apr_posix_perms2mode(perm),
                          DB_HASH, NULL);
        if (file.bdb == NULL)
            return APR_EGENERAL;      /* ### need a better error */
        dberr = 0;
#endif
        if (dberr != 0)
            return db2s(dberr);
    }

#else
#error apr_dbm_open has not been coded for this database type
#endif /* switch on database types */

    /* we have an open database... return it */
    *pdb = apr_pcalloc(pool, sizeof(**pdb));
    (*pdb)->pool = pool;
    (*pdb)->type = &DBM_VTABLE;
    SET_FILE(*pdb, file);

    /* ### register a cleanup to close the DBM? */

    return APR_SUCCESS;
}

APU_DECLARE(void) apr_dbm_close(apr_dbm_t *dbm)
{
    (*dbm->type->close)(dbm);
}

APU_DECLARE(apr_status_t) apr_dbm_fetch(apr_dbm_t *dbm, apr_datum_t key,
                                        apr_datum_t *pvalue)
{
    apr_status_t rv;
    cvt_datum_t ckey;
    result_datum_t rd;

    CONVERT_DATUM(ckey, &key);
#if APU_USE_DB
    memset(&rd,0,sizeof(rd));
#endif
    rv = APR_DBM_FETCH(dbm->file, ckey, rd);
    RETURN_DATUM(pvalue, rd);

    REGISTER_CLEANUP(dbm, pvalue);

    /* store the error info into DBM, and return a status code. Also, note
       that *pvalue should have been cleared on error. */
    return set_error(dbm, rv);
}

APU_DECLARE(apr_status_t) apr_dbm_store(apr_dbm_t *dbm, apr_datum_t key,
                                        apr_datum_t value)
{
    apr_status_t rv;
    cvt_datum_t ckey;
    cvt_datum_t cvalue;

    CONVERT_DATUM(ckey, &key);
    CONVERT_DATUM(cvalue, &value);
    rv = APR_DBM_STORE(dbm->file, ckey, cvalue);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

APU_DECLARE(apr_status_t) apr_dbm_delete(apr_dbm_t *dbm, apr_datum_t key)
{
    apr_status_t rv;
    cvt_datum_t ckey;

    CONVERT_DATUM(ckey, &key);
    rv = APR_DBM_DELETE(dbm->file, ckey);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

APU_DECLARE(int) apr_dbm_exists(apr_dbm_t *dbm, apr_datum_t key)
{
    int exists;
    cvt_datum_t ckey;

    CONVERT_DATUM(ckey, &key);

#if APU_USE_SDBM
    {
	apr_sdbm_datum_t value;
        if (apr_sdbm_fetch(dbm->file, &value, *ckey) != APR_SUCCESS) {
	    exists = 0;
        }
        else
            exists = value.dptr != NULL;
    }
#elif APU_USE_GDBM
    exists = gdbm_exists(dbm->file, *ckey) != 0;
#elif APU_USE_DB
    {
        DBT data;
        int dberr = do_fetch(GET_BDB(dbm->file), ckey, data);

        /* DB returns DB_NOTFOUND if it doesn't exist. but we want to say
           that *any* error means it doesn't exist. */
        exists = dberr == 0;
    }
#else
#error apr_dbm_exists has not been coded for this database type
#endif
    return exists;
}

APU_DECLARE(apr_status_t) apr_dbm_firstkey(apr_dbm_t *dbm, apr_datum_t *pkey)
{
    apr_status_t rv;
    result_datum_t rd;

    rv = APR_DBM_FIRSTKEY(dbm->file, rd);
    RETURN_DATUM(pkey, rd);

    REGISTER_CLEANUP(dbm, pkey);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

APU_DECLARE(apr_status_t) apr_dbm_nextkey(apr_dbm_t *dbm, apr_datum_t *pkey)
{
    apr_status_t rv;
    cvt_datum_t ckey;
    result_datum_t rd;

    CONVERT_DATUM(ckey, pkey);
    rv = APR_DBM_NEXTKEY(dbm->file, ckey, rd);
    RETURN_DATUM(pkey, rd);

    REGISTER_CLEANUP(dbm, pkey);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, APR_SUCCESS);
}

APU_DECLARE(void) apr_dbm_freedatum(apr_dbm_t *dbm, apr_datum_t data)
{
    (*dbm->type->freedatum)(dbm, data);
}

APU_DECLARE(char *) apr_dbm_geterror(apr_dbm_t *dbm, int *errcode,
                                     char *errbuf, apr_size_t errbufsize)
{
    if (errcode != NULL)
        *errcode = dbm->errcode;

    /* assert: errbufsize > 0 */

    if (dbm->errmsg == NULL)
        *errbuf = '\0';
    else
        (void) apr_cpystrn(errbuf, dbm->errmsg, errbufsize);
    return errbuf;
}

APU_DECLARE(void) apr_dbm_get_usednames(apr_pool_t *p,
                                        const char *pathname,
                                        const char **used1,
                                        const char **used2)
{
    /* ### one day, we will pass in a DBM name and need to look it up.
       ### for now, it is constant. */

    (*DBM_VTABLE.getusednames)(p, pathname, used1, used2);
}

/* Most DBM libraries take a POSIX mode for creating files.  Don't trust
 * the mode_t type, some platforms may not support it, int is safe.
 */
APU_DECLARE(int) apr_posix_perms2mode(apr_fileperms_t perm)
{
    int mode = 0;

    mode |= 0700 & (perm >> 2); /* User  is off-by-2 bits */
    mode |= 0070 & (perm >> 1); /* Group is off-by-1 bit */
    mode |= 0007 & (perm);      /* World maps 1 for 1 */
    return mode;
}
