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

#include "apr_dbm_private.h"

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

/* ### in apr_dbm_freedatum(), run the cleanup */
#define NEEDS_CLEANUP

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

/* --------------------------------------------------------------------------
**
** DEFINE THE VTABLE FUNCTIONS FOR GDBM
*/

static apr_status_t vt_gdbm_open(apr_dbm_t **dbm, const char *name,
                                 apr_int32_t mode, apr_fileperms_t perm,
                                 apr_pool_t *cntxt)
{
    abort();
    return APR_SUCCESS;
}

static void vt_gdbm_close(apr_dbm_t *dbm)
{
    abort();
}

static apr_status_t vt_gdbm_fetch(apr_dbm_t *dbm, apr_datum_t key,
                                  apr_datum_t * pvalue)
{
    abort();
    return APR_SUCCESS;
}

static apr_status_t vt_gdbm_store(apr_dbm_t *dbm, apr_datum_t key,
                                  apr_datum_t value)
{
    abort();
    return APR_SUCCESS;
}

static apr_status_t vt_gdbm_del(apr_dbm_t *dbm, apr_datum_t key)
{
    abort();
    return APR_SUCCESS;
}

static int vt_gdbm_exists(apr_dbm_t *dbm, apr_datum_t key)
{
    abort();
    return 0;
}

static apr_status_t vt_gdbm_firstkey(apr_dbm_t *dbm, apr_datum_t * pkey)
{
    abort();
    return APR_SUCCESS;
}

static apr_status_t vt_gdbm_nextkey(apr_dbm_t *dbm, apr_datum_t * pkey)
{
    abort();
    return APR_SUCCESS;
}

static char * vt_gdbm_geterror(apr_dbm_t *dbm, int *errcode, char *errbuf,
                               apr_size_t errbufsize)
{
    abort();
    return NULL;
}

static void vt_gdbm_freedatum(apr_dbm_t *dbm, apr_datum_t data)
{
    abort();
}

static void vt_gdbm_usednames(apr_pool_t *pool, const char *pathname,
                              const char **used1, const char **used2)
{
    abort();
}


static const apr_dbm_type_t apr_dbm_type_gdbm = {
    "gdbm",

    vt_gdbm_open,
    vt_gdbm_close,
    vt_gdbm_fetch,
    vt_gdbm_store,
    vt_gdbm_del,
    vt_gdbm_exists,
    vt_gdbm_firstkey,
    vt_gdbm_nextkey,
    vt_gdbm_geterror,
    vt_gdbm_freedatum,
    vt_gdbm_usednames
};
