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

#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_strings.h"

#include "apu_private.h"
#include "apr_dbm.h"

#if APU_USE_SDBM
#include "apr_sdbm.h"

typedef SDBM *real_file_t;
typedef sdbm_datum real_datum_t;

#define APR_DBM_CLOSE(f)	sdbm_close(f)
#define APR_DBM_FETCH(f, k)	sdbm_fetch((f), (k))
#define APR_DBM_STORE(f, k, v)	sdbm_store((f), (k), (v), SDBM_REPLACE)
#define APR_DBM_DELETE(f, k)	sdbm_delete((f), (k))
#define APR_DBM_FIRSTKEY(f)	sdbm_firstkey(f)
#define APR_DBM_NEXTKEY(f, k)	sdbm_nextkey(f)
#define APR_DBM_FREEDPTR(dptr)	if (0) ; else	/* stop "no effect" warning */

#define APR_DBM_DBMODE_RO       APR_READ
#define APR_DBM_DBMODE_RW       (APR_READ | APR_WRITE)
#define APR_DBM_DBMODE_RWCREATE (APR_READ | APR_WRITE | APR_CREATE)

#elif APU_USE_GDBM
#include <gdbm.h>
#include <stdlib.h>     /* for free() */

typedef GDBM_FILE real_file_t;
typedef datum real_datum_t;

#define APR_DBM_CLOSE(f)	gdbm_close(f)
#define APR_DBM_FETCH(f, k)	gdbm_fetch((f), (k))
#define APR_DBM_STORE(f, k, v)	g2s(gdbm_store((f), (k), (v), GDBM_REPLACE))
#define APR_DBM_DELETE(f, k)	g2s(gdbm_delete((f), (k)))
#define APR_DBM_FIRSTKEY(f)	gdbm_firstkey(f)
#define APR_DBM_NEXTKEY(f, k)	gdbm_nextkey((f), (k))
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

#else
#error a DBM implementation was not specified
#endif


struct apr_dbm_t
{
    apr_pool_t *pool;
    real_file_t file;

    int errcode;
    const char *errmsg;
};

/* apr_datum <=> real_datum casting/conversions */
#define A2R_DATUM(d)    (*(real_datum_t *)&(d))
#define R2A_DATUM(d)    (*(apr_datum_t *)&(d))


#ifdef NEEDS_CLEANUP

static apr_status_t datum_cleanup(void *dptr)
{
    APR_DBM_FREEDPTR(dptr);
    return APR_SUCCESS;
}

#define REG_CLEANUP(dbm, pdatum) \
    if ((pdatum)->dptr) \
        apr_register_cleanup((dbm)->pool, (pdatum)->dptr, \
                             datum_cleanup, apr_null_cleanup); \
    else

#else /* NEEDS_CLEANUP */

#define REG_CLEANUP(dbm, pdatum) if (0) ; else   /* stop "no effect" warning */

#endif /* NEEDS_CLEANUP */

static apr_status_t set_error(apr_dbm_t *dbm, apr_status_t dbm_said)
{
    apr_status_t rv = APR_SUCCESS;

    /* ### ignore whatever the DBM said (dbm_said); ask it explicitly */

#if APU_USE_SDBM

    if ((dbm->errcode = sdbm_error(dbm->file)) == 0) {
        dbm->errmsg = NULL;
    }
    else {
        dbm->errmsg = "I/O error occurred.";
        rv = APR_EGENERAL;        /* ### need something better */
    }

    /* captured it. clear it now. */
    sdbm_clearerr(dbm->file);

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

#endif

    return rv;
}

APU_DECLARE(apr_status_t) apr_dbm_open(apr_dbm_t **pdb, const char *pathname, 
                                       int mode, apr_pool_t *pool)
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

        rv = sdbm_open(&file, pathname, dbmode, APR_OS_DEFAULT, pool);
        if (rv != APR_SUCCESS)
            return rv;
    }
#elif APU_USE_GDBM
    {
        /* Note: stupid cast to get rid of "const" on the pathname */
        file = gdbm_open((char *) pathname, 0, dbmode, 0660, NULL);
        if (file == NULL)
            return APR_EINVAL;      /* ### need a better error */
    }
#endif

    /* we have an open database... return it */
    *pdb = apr_pcalloc(pool, sizeof(**pdb));
    (*pdb)->pool = pool;
    (*pdb)->file = file;

    return APR_SUCCESS;
}

APU_DECLARE(void) apr_dbm_close(apr_dbm_t *dbm)
{
    APR_DBM_CLOSE(dbm->file);
}

APU_DECLARE(apr_status_t) apr_dbm_fetch(apr_dbm_t *dbm, apr_datum_t key,
                                        apr_datum_t *pvalue)
{
    *(real_datum_t *) pvalue = APR_DBM_FETCH(dbm->file, A2R_DATUM(key));

    REG_CLEANUP(dbm, pvalue);

    /* store the error info into DBM, and return a status code. Also, note
       that *pvalue should have been cleared on error. */
    return set_error(dbm, APR_SUCCESS);
}

APU_DECLARE(apr_status_t) apr_dbm_store(apr_dbm_t *dbm, apr_datum_t key,
                                        apr_datum_t value)
{
    apr_status_t rv;

    rv = APR_DBM_STORE(dbm->file, A2R_DATUM(key), A2R_DATUM(value));

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

APU_DECLARE(apr_status_t) apr_dbm_delete(apr_dbm_t *dbm, apr_datum_t key)
{
    apr_status_t rv;

    rv = APR_DBM_DELETE(dbm->file, A2R_DATUM(key));

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

APU_DECLARE(int) apr_dbm_exists(apr_dbm_t *dbm, apr_datum_t key)
{
    int exists;

#if APU_USE_SDBM
    {
	sdbm_datum value = sdbm_fetch(dbm->file, A2R_DATUM(key));
	sdbm_clearerr(dbm->file);	/* don't need the error */
	exists = value.dptr != NULL;
    }
#elif APU_USE_GDBM
    exists = gdbm_exists(dbm->file, A2R_DATUM(key)) != 0;
#endif
    return exists;
}

APU_DECLARE(apr_status_t) apr_dbm_firstkey(apr_dbm_t *dbm, apr_datum_t *pkey)
{
    *(real_datum_t *) pkey = APR_DBM_FIRSTKEY(dbm->file);

    REG_CLEANUP(dbm, pkey);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, APR_SUCCESS);
}

APU_DECLARE(apr_status_t) apr_dbm_nextkey(apr_dbm_t *dbm, apr_datum_t *pkey)
{
    *(real_datum_t *) pkey = APR_DBM_NEXTKEY(dbm->file, A2R_DATUM(*pkey));

    REG_CLEANUP(dbm, pkey);

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, APR_SUCCESS);
}

APU_DECLARE(void) apr_dbm_freedatum(apr_dbm_t *dbm, apr_datum_t data)
{
#ifdef NEEDS_CLEANUP
    (void) apr_run_cleanup(dbm->pool, data.dptr, datum_cleanup);
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
#elif APU_USE_GDBM
    *used1 = apr_pstrdup(p, pathname);
    *used2 = NULL;
#endif
}
