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
#define APR_DBM_DBMODE_RWTRUNC (O_CREAT | O_RDWR|O_TRUNC)
#else
#define APR_DBM_DBMODE_RO       DB_RDONLY
#define APR_DBM_DBMODE_RW       0
#define APR_DBM_DBMODE_RWCREATE DB_CREATE
#define APR_DBM_DBMODE_RWTRUNC  DB_TRUNCATE
#endif /* DBVER == 1 */

/* map a DB error to an apr_status_t */
static apr_status_t db2s(int dberr)
{
    if (dberr != 0) {
        /* ### need to fix this */
        return APR_OS_START_USEERR+dberr;
    }

    return APR_SUCCESS;
}

/* handle the FIRSTKEY functionality */
static apr_status_t do_firstkey(real_file_t *f, DBT *pkey)
{
    int dberr;
    DBT data;

    memset(pkey,0,sizeof(DBT));

    memset(&data,0,sizeof(DBT));
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
