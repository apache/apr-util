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

#if APU_HAVE_NDBM 
#include "apr_dbm_private.h"

#include <ndbm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* this is used in a few places to define a noop "function". it is needed
   to stop "no effect" warnings from GCC. */
#define NOOP_FUNCTION if (0) ; else

#define APR_DBM_DBMODE_RO       O_RDONLY
#define APR_DBM_DBMODE_RW       O_RDWR
#define APR_DBM_DBMODE_RWCREATE (O_RDWR|O_CREAT)
#define APR_DBM_DBMODE_RWTRUNC  (O_RDWR|O_CREAT|O_TRUNC)

/* map a NDBM error to an apr_status_t */
static apr_status_t ndbm2s(int ndbmerr)
{
    if (ndbmerr == -1) {
        /* ### need to fix this */
        return APR_EGENERAL;
    }

    return APR_SUCCESS;
}

static apr_status_t set_error(apr_dbm_t *dbm, apr_status_t dbm_said)
{
    apr_status_t rv = APR_SUCCESS;

    /* ### ignore whatever the DBM said (dbm_said); ask it explicitly */

    dbm->errmsg = NULL;
    if (dbm_error((DBM*)dbm->file)) {
        dbm->errmsg = NULL;
        rv = APR_EGENERAL;        /* ### need something better */
    }

    /* captured it. clear it now. */
    dbm_clearerr((DBM*)dbm->file);

    return rv;
}

/* --------------------------------------------------------------------------
**
** DEFINE THE VTABLE FUNCTIONS FOR NDBM
*/

static apr_status_t vt_ndbm_open(apr_dbm_t **pdb, const char *pathname,
                                 apr_int32_t mode, apr_fileperms_t perm,
                                 apr_pool_t *pool)
{
    DBM *file;
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
        file = dbm_open(pathname, dbmode, apr_posix_perms2mode(perm));
        if (file == NULL)
            return APR_EGENERAL;      /* ### need a better error */
    }

    /* we have an open database... return it */
    *pdb = apr_pcalloc(pool, sizeof(**pdb));
    (*pdb)->pool = pool;
    (*pdb)->type = &apr_dbm_type_ndbm;
    (*pdb)->file = file;

    /* ### register a cleanup to close the DBM? */

    return APR_SUCCESS;
}

static void vt_ndbm_close(apr_dbm_t *dbm)
{
    dbm_close(dbm->file);
}

static apr_status_t vt_ndbm_fetch(apr_dbm_t *dbm, apr_datum_t key,
                                  apr_datum_t * pvalue)
{
    datum *ckey;
    datum rd;

    ckey = (datum*)&key;
    rd = dbm_fetch(dbm->file, *ckey);
    *pvalue = *(apr_datum_t*)&rd;

    /* store the error info into DBM, and return a status code. Also, note
       that *pvalue should have been cleared on error. */
    return set_error(dbm, APR_SUCCESS);
}

static apr_status_t vt_ndbm_store(apr_dbm_t *dbm, apr_datum_t key,
                                  apr_datum_t value)
{
    apr_status_t rv;
    datum *ckey;
    datum *cvalue;

    ckey =  (datum*)&key;
    cvalue = (datum*)&value;
    rv = ndbm2s( dbm_store( dbm->file, *ckey, *cvalue, DBM_REPLACE));

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

static apr_status_t vt_ndbm_del(apr_dbm_t *dbm, apr_datum_t key)
{
    apr_status_t rv;
    datum *ckey;

    ckey = (datum*)&key;
    rv = ndbm2s( dbm_delete(dbm->file, *ckey));

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, rv);
}

static int vt_ndbm_exists(apr_dbm_t *dbm, apr_datum_t key)
{
    datum *ckey = (datum *)&key;
    datum value;

    value = dbm_fetch( dbm->file, *ckey);

    return value.dptr != NULL;
}

static apr_status_t vt_ndbm_firstkey(apr_dbm_t *dbm, apr_datum_t * pkey)
{
    datum rd;

    rd = dbm_firstkey(dbm->file);
    *pkey = *(apr_datum_t*)&rd;

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, APR_SUCCESS);
}

static apr_status_t vt_ndbm_nextkey(apr_dbm_t *dbm, apr_datum_t * pkey)
{
    datum *ckey;
    datum rd;

    ckey = (datum*)pkey;
    rd = dbm_nextkey(dbm->file);
    *pkey = *(apr_datum_t*)&rd;

    /* store any error info into DBM, and return a status code. */
    return set_error(dbm, APR_SUCCESS);
}

static void vt_ndbm_freedatum(apr_dbm_t *dbm, apr_datum_t data)
{
  /* nothing to do */
}

static void vt_ndbm_usednames(apr_pool_t *pool, const char *pathname,
                              const char **used1, const char **used2)
{
    *used1 = apr_pstrdup(pool, pathname);
    *used2 = NULL;
}


APU_DECLARE_DATA const apr_dbm_type_t apr_dbm_type_ndbm = {
    "ndbm",

    vt_ndbm_open,
    vt_ndbm_close,
    vt_ndbm_fetch,
    vt_ndbm_store,
    vt_ndbm_del,
    vt_ndbm_exists,
    vt_ndbm_firstkey,
    vt_ndbm_nextkey,
    vt_ndbm_freedatum,
    vt_ndbm_usednames
};
#endif /* APU_HAVE_NDBM  */
