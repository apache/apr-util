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

#include "apu_select_dbm.h"
#include "apr_dbm.h"


/* this is used in a few places to define a noop "function". it is needed
   to stop "no effect" warnings from GCC. */
#define NOOP_FUNCTION if (0) ; else


#if APU_USE_SDBM

#include "apr_sdbm.h"

typedef apr_sdbm_t *real_file_t;

typedef apr_sdbm_datum_t *cvt_datum_t;
#define CONVERT_DATUM(cvt, pinput) ((cvt) = (apr_sdbm_datum_t *)(pinput))

typedef apr_sdbm_datum_t result_datum_t;
#define RETURN_DATUM(poutput, rd) (*(poutput) = *(apr_datum_t *)&(rd))

#define APR_DBM_CLOSE(f)	apr_sdbm_close(f)
#define APR_DBM_FETCH(f, k, v)	apr_sdbm_fetch(f, &(v), *(k))
#define APR_DBM_STORE(f, k, v)	apr_sdbm_store(f, *(k), *(v), APR_SDBM_REPLACE)
#define APR_DBM_DELETE(f, k)	apr_sdbm_delete(f, *(k))
#define APR_DBM_FIRSTKEY(f, k)	apr_sdbm_firstkey(f, &(k))
#define APR_DBM_NEXTKEY(f, k, nk) apr_sdbm_nextkey(f, &(nk))
#define APR_DBM_FREEDPTR(dptr)	NOOP_FUNCTION

#define APR_DBM_DBMODE_RO       APR_READ
#define APR_DBM_DBMODE_RW       (APR_READ | APR_WRITE)
#define APR_DBM_DBMODE_RWCREATE (APR_READ | APR_WRITE | APR_CREATE)

#else /* Not using SDBM: */

/* Most DBM libraries take a POSIX mode for creating files.  Don't trust
 * the mode_t type, some platforms may not support it, int is safe.
 */
int apr_posix_perms2mode(apr_fileperms_t perm) 
{
    int mode = 0;

    mode |= 0700 & (perm >> 2); /* User  is off-by-2 bits */
    mode |= 0070 & (perm >> 1); /* Group is off-by-1 bit */
    mode |= 0007 & (perm);      /* World maps 1 for 1 */
    return mode;
}

#if APU_USE_GDBM

#include <gdbm.h>
#include <stdlib.h>     /* for free() */

typedef GDBM_FILE real_file_t;

typedef datum *cvt_datum_t;
#define CONVERT_DATUM(cvt, pinput) ((cvt) = (datum *)(pinput))

typedef datum result_datum_t;
#define RETURN_DATUM(poutput, rd) (*(poutput) = *(apr_datum_t *)&(rd))

#define APR_DBM_CLOSE(f)	gdbm_close(f)
#define APR_DBM_FETCH(f, k, v)	((v) = gdbm_fetch(f, *(k)), APR_SUCCESS)
#define APR_DBM_STORE(f, k, v)	g2s(gdbm_store(f, *(k), *(v), GDBM_REPLACE))
#define APR_DBM_DELETE(f, k)	g2s(gdbm_delete(f, *(k)))
#define APR_DBM_FIRSTKEY(f, k)	((k) = gdbm_firstkey(f), APR_SUCCESS)
#define APR_DBM_NEXTKEY(f, k, nk) ((nk) = gdbm_nextkey(f, *(k)), APR_SUCCESS)
#define APR_DBM_FREEDPTR(dptr)	((dptr) ? free(dptr) : 0)

#define NEEDS_CLEANUP

#define APR_DBM_DBMODE_RO       GDBM_READER
#define APR_DBM_DBMODE_RW       GDBM_WRITER
#define APR_DBM_DBMODE_RWCREATE GDBM_WRCREAT

/* map a GDBM error to an apr_status_t */
static apr_status_t g2s(int gerr)
{
    if (gerr == -1) {
        /* ### need to fix this */
        return APR_EGENERAL;
    }

    return APR_SUCCESS;
}

#elif APU_USE_DB
/*
 * We pick up all varieties of Berkeley DB through db.h (included through
 * apu_select_dbm.h). This code has been compiled/tested against DB1,
 * DB_185, DB2, and DB3.
 */

#if   defined(DB_VERSION_MAJOR) && (DB_VERSION_MAJOR == 3)
#define DB_VER 3
#elif defined(DB_VERSION_MAJOR) && (DB_VERSION_MAJOR == 2)
#define DB_VER 2
#else
#define DB_VER 1
#endif

typedef struct {
    DB *bdb;
#if DB_VER != 1
    DBC *curs;
#endif
} real_file_t;

typedef DBT cvt_datum_t;
#define CONVERT_DATUM(cvt, pinput) (memset(&(cvt), 0, sizeof(cvt)), \
                                    (cvt).data = (pinput)->dptr, \
                                    (cvt).size = (pinput)->dsize)

typedef DBT result_datum_t;
#define RETURN_DATUM(poutput, rd) ((poutput)->dptr = (rd).data, \
                                   (poutput)->dsize = (rd).size)

#if DB_VER == 1
#define TXN_ARG
#else
#define TXN_ARG NULL,
#endif

#if DB_VER == 1
#define APR_DBM_CLOSE(f)	((*(f).bdb->close)((f).bdb))
#else
#define APR_DBM_CLOSE(f)	((*(f).bdb->close)((f).bdb, 0))
#endif

#define do_fetch(f, k, v)       ((*(f)->get)(f, TXN_ARG &(k), &(v), 0))
#define APR_DBM_FETCH(f, k, v)	db2s(do_fetch((f).bdb, k, v))
#define APR_DBM_STORE(f, k, v)	db2s((*(f).bdb->put)((f).bdb, TXN_ARG &(k), &(v), 0))
#define APR_DBM_DELETE(f, k)	db2s((*(f).bdb->del)((f).bdb, TXN_ARG &(k), 0))
#define APR_DBM_FIRSTKEY(f, k)  do_firstkey(&(f), &(k))
#define APR_DBM_NEXTKEY(f, k, nk) do_nextkey(&(f), &(k), &(nk))
#define APR_DBM_FREEDPTR(dptr)	NOOP_FUNCTION

#if DB_VER == 1
#include <sys/fcntl.h>
#define APR_DBM_DBMODE_RO       O_RDONLY
#define APR_DBM_DBMODE_RW       O_RDWR
#define APR_DBM_DBMODE_RWCREATE (O_CREAT | O_RDWR)
#else
#define APR_DBM_DBMODE_RO       DB_RDONLY
#define APR_DBM_DBMODE_RW       0
#define APR_DBM_DBMODE_RWCREATE DB_CREATE
#endif

/* map a DB error to an apr_status_t */
static apr_status_t db2s(int dberr)
{
    if (dberr != 0) {
        /* ### need to fix this */
        return APR_EGENERAL;
    }

    return APR_SUCCESS;
}

/* handle the FIRSTKEY functionality */
static apr_status_t do_firstkey(real_file_t *f, DBT *pkey)
{
    int dberr;
    DBT data;

#if DB_VER == 1
    dberr = (*f->bdb->seq)(f->bdb, pkey, &data, R_FIRST);
#else
    if ((dberr = (*f->bdb->cursor)(f->bdb, NULL, &f->curs
#if DB_VER == 3
                                   , 0
#endif
                                   )) == 0) {
        dberr = (*f->curs->c_get)(f->curs, pkey, &data, DB_FIRST);
        if (dberr == DB_NOTFOUND) {
            memset(pkey, 0, sizeof(*pkey));
            (*f->curs->c_close)(f->curs);
            f->curs = NULL;
            return APR_SUCCESS;
        }
    }
#endif

    return db2s(dberr);
}

/* handle the NEXTKEY functionality */
static apr_status_t do_nextkey(real_file_t *f, DBT *pkey, DBT *pnext)
{
    int dberr;
    DBT data;

#if DB_VER == 1
    dberr = (*f->bdb->seq)(f->bdb, pkey, &data, R_NEXT);
#else
    if (f->curs == NULL)
        return APR_EINVAL;

    dberr = (*f->curs->c_get)(f->curs, pkey, &data, DB_NEXT);
    if (dberr == DB_NOTFOUND) {
        memset(pkey, 0, sizeof(*pkey));
        (*f->curs->c_close)(f->curs);
        f->curs = NULL;
        return APR_SUCCESS;
    }
#endif

    return db2s(dberr);
}

#else /* Not in the USE_xDBM list above */
#error a DBM implementation was not specified
#endif

#endif /* Not USE_SDBM */

struct apr_dbm_t
{
    apr_pool_t *pool;
    real_file_t file;

    int errcode;
    const char *errmsg;
};


#ifdef NEEDS_CLEANUP

static apr_status_t datum_cleanup(void *dptr)
{
    APR_DBM_FREEDPTR(dptr);
    return APR_SUCCESS;
}

#define REGISTER_CLEANUP(dbm, pdatum) \
    if ((pdatum)->dptr) \
        apr_pool_cleanup_register((dbm)->pool, (pdatum)->dptr, \
                             datum_cleanup, apr_pool_cleanup_null); \
    else

#else /* NEEDS_CLEANUP */

#define REGISTER_CLEANUP(dbm, pdatum) NOOP_FUNCTION

#endif /* NEEDS_CLEANUP */

static apr_status_t set_error(apr_dbm_t *dbm, apr_status_t dbm_said)
{
    apr_status_t rv = APR_SUCCESS;

    /* ### ignore whatever the DBM said (dbm_said); ask it explicitly */

#if APU_USE_SDBM

    if ((dbm->errcode = apr_sdbm_error_get(dbm->file)) == 0) {
        dbm->errmsg = NULL;
    }
    else {
        dbm->errmsg = "I/O error occurred.";
        rv = APR_EGENERAL;        /* ### need something better */
    }

    /* captured it. clear it now. */
    apr_sdbm_error_clear(dbm->file);

#elif APU_USE_GDBM

    if ((dbm->errcode = gdbm_errno) == GDBM_NO_ERROR) {
        dbm->errmsg = NULL;
    }
    else {
        dbm->errmsg = gdbm_strerror(gdbm_errno);
        rv = APR_EGENERAL;        /* ### need something better */
    }

    /* captured it. clear it now. */
    gdbm_errno = GDBM_NO_ERROR;

#elif APU_USE_DB

    if (dbm_said == APR_SUCCESS) {
        dbm->errcode = 0;
        dbm->errmsg = NULL;
    }
    else {
        /* ### need to fix. dberr was tossed in db2s(). */
        /* ### use db_strerror() */
        dbm->errcode = 1;
        dbm->errmsg = "DB error occurred.";
        rv = APR_EGENERAL;
    }
#else
#error set_error has not been coded for this database type
#endif

    return rv;
}

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
    (*pdb)->file = file;

    /* ### register a cleanup to close the DBM? */

    return APR_SUCCESS;
}

APU_DECLARE(void) apr_dbm_close(apr_dbm_t *dbm)
{
    APR_DBM_CLOSE(dbm->file);
}

APU_DECLARE(apr_status_t) apr_dbm_fetch(apr_dbm_t *dbm, apr_datum_t key,
                                        apr_datum_t *pvalue)
{
    apr_status_t rv;
    cvt_datum_t ckey;
    result_datum_t rd;

    CONVERT_DATUM(ckey, &key);
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
	    apr_sdbm_error_clear(dbm->file);	/* don't need the error */
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
        int dberr = do_fetch(dbm->file.bdb, ckey, data);

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
#ifdef NEEDS_CLEANUP
    (void) apr_pool_cleanup_run(dbm->pool, data.dptr, datum_cleanup);
#else
    APR_DBM_FREEDPTR(data.dptr);
#endif
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
#if APU_USE_SDBM
    char *work;

    *used1 = apr_pstrcat(p, pathname, SDBM_DIRFEXT, NULL);
    *used2 = work = apr_pstrdup(p, *used1);

    /* we know the extension is 4 characters */
    memcpy(&work[strlen(work) - 4], SDBM_PAGFEXT, 4);
#elif APU_USE_GDBM || APU_USE_DB
    *used1 = apr_pstrdup(p, pathname);
    *used2 = NULL;
#else
#error apr_dbm_get_usednames has not been coded for this database type
#endif
}
