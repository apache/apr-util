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

#include "apr_strings.h"

#if APR_HAVE_STDLIB_H
#include <stdlib.h>     /* for free() */
#endif

#include "apu.h"

#if APU_HAVE_GDBM 
#include "apr_dbm_private.h"

#include <gdbm.h>

/* this is used in a few places to define a noop "function". it is needed
   to stop "no effect" warnings from GCC. */
#define NOOP_FUNCTION if (0) ; else

/* ### define defaults for now; these will go away in a while */
#define REGISTER_CLEANUP(dbm, pdatum) NOOP_FUNCTION
#define SET_FILE(pdb, f) ((pdb)->file = (f))

typedef GDBM_FILE real_file_t;

typedef datum *cvt_datum_t;
#define CONVERT_DATUM(cvt, pinput) ((cvt) = (datum *)(pinput))

typedef datum result_datum_t;
#define RETURN_DATUM(poutput, rd) (*(poutput) = *(apr_datum_t *)&(rd))

#define APR_DBM_CLOSE(f)        gdbm_close(f)
#define APR_DBM_FETCH(f, k, v)  ((v) = gdbm_fetch(f, *(k)), APR_SUCCESS)
#define APR_DBM_STORE(f, k, v)  g2s(gdbm_store(f, *(k), *(v), GDBM_REPLACE))
#define APR_DBM_DELETE(f, k)    g2s(gdbm_delete(f, *(k)))
#define APR_DBM_FIRSTKEY(f, k)  ((k) = gdbm_firstkey(f), APR_SUCCESS)
#define APR_DBM_NEXTKEY(f, k, nk) ((nk) = gdbm_nextkey(f, *(k)), APR_SUCCESS)
#define APR_DBM_FREEDPTR(dptr)  ((dptr) ? free(dptr) : 0)

#undef REGISTER_CLEANUP
#define REGISTER_CLEANUP(dbm, pdatum) \
    if ((pdatum)->dptr) \
        apr_pool_cleanup_register((dbm)->pool, (pdatum)->dptr, \
                             datum_cleanup, apr_pool_cleanup_null); \
    else

#define APR_DBM_DBMODE_RO       GDBM_READER
#define APR_DBM_DBMODE_RW       GDBM_WRITER
#define APR_DBM_DBMODE_RWCREATE GDBM_WRCREAT
#define APR_DBM_DBMODE_RWTRUNC  GDBM_NEWDB

/* map a GDBM error to an apr_status_t */
static apr_status_t g2s(int gerr)
{
    if (gerr == -1) {
        /* ### need to fix this */
        return APR_EGENERAL;
    }

    return APR_SUCCESS;
}

static apr_status_t datum_cleanup(void *dptr)
{
    if (dptr)
        free(dptr);

    return APR_SUCCESS;
}

static apr_status_t set_error(apr_dbm_t *dbm, apr_status_t dbm_said)
{
    apr_status_t rv = APR_SUCCESS;

    /* ### ignore whatever the DBM said (dbm_said); ask it explicitly */

    if ((dbm->errcode = gdbm_errno) == GDBM_NO_ERROR) {
        dbm->errmsg = NULL;
    }
    else {
        dbm->errmsg = gdbm_strerror(gdbm_errno);
        rv = APR_EGENERAL;        /* ### need something better */
    }

    /* captured it. clear it now. */
    gdbm_errno = GDBM_NO_ERROR;

    return rv;
}

/* --------------------------------------------------------------------------
**
** DEFINE THE VTABLE FUNCTIONS FOR GDBM
*/

static apr_status_t vt_gdbm_open(apr_dbm_t **pdb, const char *pathname,
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

    {
        /* Note: stupid cast to get rid of "const" on the pathname */
        file = gdbm_open((char *) pathname, 0, dbmode,
                         apr_posix_perms2mode(perm), NULL);
        if (file == NULL)
            return APR_EGENERAL;      /* ### need a better error */
    }

    /* we have an open database... return it */
    *pdb = apr_pcalloc(pool, sizeof(**pdb));
    (*pdb)->pool = pool;
    (*pdb)->type = &apr_dbm_type_gdbm;
    SET_FILE(*pdb, file);

    /* ### register a cleanup to close the DBM? */

    return APR_SUCCESS;
}

static void vt_gdbm_close(apr_dbm_t *dbm)
{
    APR_DBM_CLOSE(dbm->file);
}

static apr_status_t vt_gdbm_fetch(apr_dbm_t *dbm, apr_datum_t key,
                                  apr_datum_t * pvalue)
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

static apr_status_t vt_gdbm_store(apr_dbm_t *dbm, apr_datum_t key,
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

static apr_status_t vt_gdbm_del(apr_dbm_t *dbm, apr_datum_t key)
{
    apr_status_t rv;
    cvt_datum_t ckey;

    CONVERT_DATUM(ckey, &key);
    rv = APR_DBM_DELETE(dbm->file, ckey);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

static int vt_gdbm_exists(apr_dbm_t *dbm, apr_datum_t key)
{
    datum *ckey = (datum *)&key;

    return gdbm_exists(dbm->file, *ckey) != 0;
}

static apr_status_t vt_gdbm_firstkey(apr_dbm_t *dbm, apr_datum_t * pkey)
{
    apr_status_t rv;
    result_datum_t rd;

    rv = APR_DBM_FIRSTKEY(dbm->file, rd);
    RETURN_DATUM(pkey, rd);

    REGISTER_CLEANUP(dbm, pkey);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

static apr_status_t vt_gdbm_nextkey(apr_dbm_t *dbm, apr_datum_t * pkey)
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

static void vt_gdbm_freedatum(apr_dbm_t *dbm, apr_datum_t data)
{
    (void) apr_pool_cleanup_run(dbm->pool, data.dptr, datum_cleanup);
}

static void vt_gdbm_usednames(apr_pool_t *pool, const char *pathname,
                              const char **used1, const char **used2)
{
    *used1 = apr_pstrdup(pool, pathname);
    *used2 = NULL;
}


APU_DECLARE_DATA const apr_dbm_type_t apr_dbm_type_gdbm = {
    "gdbm",

    vt_gdbm_open,
    vt_gdbm_close,
    vt_gdbm_fetch,
    vt_gdbm_store,
    vt_gdbm_del,
    vt_gdbm_exists,
    vt_gdbm_firstkey,
    vt_gdbm_nextkey,
    vt_gdbm_freedatum,
    vt_gdbm_usednames
};

#endif /* APU_HAVE_GDBM */
