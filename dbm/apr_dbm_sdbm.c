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
#define APR_WANT_MEMFUNC
#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "apu.h"

#if APU_HAVE_SDBM

#include "apr_dbm_private.h"

#include "apr_sdbm.h"
#if APR_HAVE_STDLIB_H
#include <stdlib.h>  /* For abort() */
#endif

/* this is used in a few places to define a noop "function". it is needed
   to stop "no effect" warnings from GCC. */
#define NOOP_FUNCTION if (0) ; else

/* ### define defaults for now; these will go away in a while */
#define REGISTER_CLEANUP(dbm, pdatum) NOOP_FUNCTION
#define SET_FILE(pdb, f) ((pdb)->file = (f))

typedef apr_sdbm_t *real_file_t;

typedef apr_sdbm_datum_t cvt_datum_t;
#define CONVERT_DATUM(cvt, pinput) ((cvt).dptr = (pinput)->dptr, (cvt).dsize = (pinput)->dsize)

typedef apr_sdbm_datum_t result_datum_t;
#define RETURN_DATUM(poutput, rd) ((poutput)->dptr = (rd).dptr, (poutput)->dsize = (rd).dsize)

#define APR_DBM_CLOSE(f)        apr_sdbm_close(f)
#define APR_DBM_FETCH(f, k, v)  apr_sdbm_fetch(f, &(v), (k))
#define APR_DBM_STORE(f, k, v)  apr_sdbm_store(f, (k), (v), APR_SDBM_REPLACE)
#define APR_DBM_DELETE(f, k)    apr_sdbm_delete(f, (k))
#define APR_DBM_FIRSTKEY(f, k)  apr_sdbm_firstkey(f, &(k))
#define APR_DBM_NEXTKEY(f, k, nk) apr_sdbm_nextkey(f, &(nk))
#define APR_DBM_FREEDPTR(dptr)  NOOP_FUNCTION

#define APR_DBM_DBMODE_RO       APR_READ
#define APR_DBM_DBMODE_RW       (APR_READ | APR_WRITE)
#define APR_DBM_DBMODE_RWCREATE (APR_READ | APR_WRITE | APR_CREATE)
#define APR_DBM_DBMODE_RWTRUNC  (APR_READ | APR_WRITE | APR_CREATE | \
                                 APR_TRUNCATE)

static apr_status_t set_error(apr_dbm_t *dbm, apr_status_t dbm_said)
{
    apr_status_t rv = APR_SUCCESS;

    /* ### ignore whatever the DBM said (dbm_said); ask it explicitly */

    if ((dbm->errcode = dbm_said) == APR_SUCCESS) {
        dbm->errmsg = NULL;
    }
    else {
        dbm->errmsg = "I/O error occurred.";
        rv = APR_EGENERAL;        /* ### need something better */
    }

    return rv;
}

/* --------------------------------------------------------------------------
**
** DEFINE THE VTABLE FUNCTIONS FOR SDBM
*/

static apr_status_t vt_sdbm_open(apr_dbm_t **pdb, const char *pathname,
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
        apr_status_t rv;

        rv = apr_sdbm_open(&file, pathname, dbmode, perm, pool);
        if (rv != APR_SUCCESS)
            return rv;
    }

    /* we have an open database... return it */
    *pdb = apr_pcalloc(pool, sizeof(**pdb));
    (*pdb)->pool = pool;
    (*pdb)->type = &apr_dbm_type_sdbm;
    SET_FILE(*pdb, file);

    /* ### register a cleanup to close the DBM? */

    return APR_SUCCESS;
}

static void vt_sdbm_close(apr_dbm_t *dbm)
{
    APR_DBM_CLOSE(dbm->file);
}

static apr_status_t vt_sdbm_fetch(apr_dbm_t *dbm, apr_datum_t key,
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

static apr_status_t vt_sdbm_store(apr_dbm_t *dbm, apr_datum_t key,
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

static apr_status_t vt_sdbm_del(apr_dbm_t *dbm, apr_datum_t key)
{
    apr_status_t rv;
    cvt_datum_t ckey;

    CONVERT_DATUM(ckey, &key);
    rv = APR_DBM_DELETE(dbm->file, ckey);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

static int vt_sdbm_exists(apr_dbm_t *dbm, apr_datum_t key)
{
    int exists;
    apr_sdbm_datum_t *ckey = (apr_sdbm_datum_t *)&key;

    {
        apr_sdbm_datum_t value;
        if (apr_sdbm_fetch(dbm->file, &value, *ckey) != APR_SUCCESS) {
            exists = 0;
        }
        else
            exists = value.dptr != NULL;
    }

    return exists;
}

static apr_status_t vt_sdbm_firstkey(apr_dbm_t *dbm, apr_datum_t * pkey)
{
    apr_status_t rv;
    result_datum_t rd;

    rv = APR_DBM_FIRSTKEY(dbm->file, rd);
    RETURN_DATUM(pkey, rd);

    REGISTER_CLEANUP(dbm, pkey);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

static apr_status_t vt_sdbm_nextkey(apr_dbm_t *dbm, apr_datum_t * pkey)
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

static void vt_sdbm_freedatum(apr_dbm_t *dbm, apr_datum_t data)
{
    APR_DBM_FREEDPTR(data.dptr);
}

static void vt_sdbm_usednames(apr_pool_t *pool, const char *pathname,
                              const char **used1, const char **used2)
{
    char *work;

    /* ### this could be optimized by computing strlen() once and using
       ### memcpy and pmemdup instead. but why bother? */

    *used1 = apr_pstrcat(pool, pathname, APR_SDBM_DIRFEXT, NULL);
    *used2 = work = apr_pstrdup(pool, *used1);

    /* we know the extension is 4 characters */
    memcpy(&work[strlen(work) - 4], APR_SDBM_PAGFEXT, 4);
}


APU_DECLARE_DATA const apr_dbm_type_t apr_dbm_type_sdbm = {
    "sdbm",

    vt_sdbm_open,
    vt_sdbm_close,
    vt_sdbm_fetch,
    vt_sdbm_store,
    vt_sdbm_del,
    vt_sdbm_exists,
    vt_sdbm_firstkey,
    vt_sdbm_nextkey,
    vt_sdbm_freedatum,
    vt_sdbm_usednames
};

#endif /* APU_HAVE_SDBM */
