/* Copyright 2005 The Apache Software Foundation or its licensors, as
 * applicable.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Developed initially by Nick Kew and Chris Darroch.
 * Contributed to the APR project by kind permission of
 * Pearson Education Core Technology Group (CTG),
 * formerly Central Media Group (CMG).
 */

/* apr_dbd_oracle - a painful attempt
 *
 * Based first on the documentation at
 * http://download-west.oracle.com/docs/cd/B10501_01/appdev.920/a96584/toc.htm
 *
 * Those docs have a lot of internal inconsistencies, contradictions, etc
 * So I've snarfed the demo programs (from Oracle 8, not included in
 * the current downloadable oracle), and used code from them.
 *
 * Why do cdemo81.c and cdemo82.c do the same thing in very different ways?
 * e.g. cdemo82 releases all its handle on shutdown; cdemo81 doesn't
 *
 * All the ORA* functions return a "sword".  Some of them are documented;
 * others aren't.  So I've adopted a policy of using switch statements
 * everywhere, even when we're not doing anything with the return values.
 *
 * This makes no attempt at performance tuning, such as setting
 * prefetch cache size.  We need some actual performance data
 * to make that meaningful.  Input from someone with experience
 * as a sysop using oracle would be a good start.
 */

/*******************************************************************/

/* GLOBAL_PREPARED_STATEMENTS
 *
 * This probably needs bindings taken away from prepare and into
 * execute, or else we'll get race conditions on the parameters
 * Might be able to do it with some pool refactoring.
 *
 * In fact, though the documentation says statements can run with
 * different interps and threads (IIRC), it doesn't say anything
 * about race conditions between bind and execute. So we'd better
 * not even try until and unless someone who knows oracle from the
 * inside fixes it.
#define GLOBAL_PREPARED_STATEMENTS
 */

/* shut compiler up */
#ifdef DEBUG
#define int_errorcode int errorcode
#else
#define int_errorcode
#endif

#include "apu.h"

#if APU_HAVE_ORACLE

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <oci.h>

#include "apr_strings.h"
#include "apr_time.h"
#include "apr_hash.h"

#define TRANS_TIMEOUT 30
#define MAX_ARG_LEN 256 /* in line with other apr_dbd drivers.  We alloc this
                         * lots of times, so a large value gets hungry.
                         * Should really make it configurable
                         */
#define DEFAULT_LONG_SIZE 4096
#define DBD_ORACLE_MAX_COLUMNS 256
#define NUMERIC_FIELD_SIZE 32

typedef enum {
        APR_DBD_ORACLE_NULL,
        APR_DBD_ORACLE_STRING,
        APR_DBD_ORACLE_INT,
        APR_DBD_ORACLE_FLOAT,
        APR_DBD_ORACLE_LOB,
        /* don't work */
        APR_DBD_ORACLE_BLOB,
        APR_DBD_ORACLE_CLOB,
} dbd_field_type;

#ifdef DEBUG
#include <stdio.h>
#endif

#include "apr_dbd_internal.h"

/* declarations */
static const char *dbd_oracle_error(apr_dbd_t *sql, int n);
static int dbd_oracle_prepare(apr_pool_t *pool, apr_dbd_t *sql,
                              const char *query, const char *label,
                              apr_dbd_prepared_t **statement);
static int outputParams(apr_dbd_t*, apr_dbd_prepared_t*);
static int dbd_oracle_pselect(apr_pool_t *pool, apr_dbd_t *sql,
                              apr_dbd_results_t **results,
                              apr_dbd_prepared_t *statement,
                              int seek, int nargs, const char **values);
static int dbd_oracle_pquery(apr_pool_t *pool, apr_dbd_t *sql,
                             int *nrows, apr_dbd_prepared_t *statement,
                             int nargs, const char **values);
static int dbd_oracle_start_transaction(apr_pool_t *pool, apr_dbd_t *sql,
                                        apr_dbd_transaction_t **trans);
static int dbd_oracle_end_transaction(apr_dbd_transaction_t *trans);

struct apr_dbd_transaction_t {
    int mode;
    enum { TRANS_NONE, TRANS_ERROR, TRANS_1, TRANS_2 } status;
    apr_dbd_t *handle;
    OCITrans *trans;
    OCISnapshot *snapshot1;
    OCISnapshot *snapshot2;
};

struct apr_dbd_results_t {
    apr_dbd_t* handle;
    unsigned int rownum;
    int seek;
    int nrows;
    apr_dbd_prepared_t *statement;
};

struct apr_dbd_t {
    sword status;
    OCIError *err;
    OCIServer *svr;
    OCISvcCtx *svc;
    OCISession *auth;
    apr_dbd_transaction_t* trans;
    apr_pool_t *pool;
    char buf[200];        /* for error messages */
    apr_size_t long_size;
};

struct apr_dbd_row_t {
    int n;
    apr_dbd_results_t *res;
    apr_pool_t *pool;
};

typedef struct {
    dbd_field_type type;
    sb2 ind;
    sb4 len;
    OCIBind *bind;
    union {
        void *raw;
        char *stringval;
        int *ival;
        double *floatval;
        OCILobLocator *lobval;
    } value;
} bind_arg;

typedef struct {
    int type;
    sb2 ind;
    ub2 len;         /* length of actual output */
    OCIDefine *defn;
    apr_size_t sz;   /* length of buf for output */
    union {
        void *raw;
        char *stringval;
        OCILobLocator *lobval;
    } buf;
    const char *name;
} define_arg;

struct apr_dbd_prepared_t {
    OCIStmt *stmt;
    int nargs;
    bind_arg *args;
    int nout;
    define_arg *out;
    apr_dbd_t *handle;
    apr_pool_t *pool;
    int type;
};

/* AFAICT from the docs, the OCIEnv thingey can be used async
 * across threads, so lets have a global one.
 *
 * We'll need shorter-lived envs to deal with requests and connections
 *
 * Hmmm, that doesn't work: we don't have a usermem framework.
 * OK, forget about using APR pools here, until we figure out
 * the right way to do it (if such a thing exists).
 */
static ub4 null = 0;
static OCIEnv *dbd_oracle_env = NULL;
#ifdef GLOBAL_PREPARED_STATEMENTS
static apr_hash_t *oracle_statements = NULL;
#endif

static apr_status_t dbd_free_lobdesc(void *lob)
{
    switch (OCIDescriptorFree(lob, OCI_DTYPE_LOB)) {
    case OCI_SUCCESS:
        return APR_SUCCESS;
    default:
        return APR_EGENERAL;
    }
}

static apr_status_t dbd_free_snapshot(void *snap)
{
    switch (OCIDescriptorFree(snap, OCI_DTYPE_SNAP)) {
    case OCI_SUCCESS:
        return APR_SUCCESS;
    default:
        return APR_EGENERAL;
    }
}

#ifdef GLOBAL_PREPARED_STATEMENTS
static apr_status_t freeStatements(void *ptr)
{
    apr_status_t rv = APR_SUCCESS;
    OCIStmt *stmt;
    apr_hash_index_t *index;
    apr_pool_t *cachepool = apr_hash_pool_get(oracle_statements);

#ifdef PREPARE2
    OCIError *err;

    if (OCIHandleAlloc(dbd_oracle_env, (dvoid**)&err, OCI_HTYPE_ERROR, 0, NULL)
       != OCI_SUCCESS) {
        return APR_EGENERAL;
    }
#endif

    for (index = apr_hash_first(cachepool, oracle_statements);
         index != NULL;
         index = apr_hash_next(index)) {
        apr_hash_this(index, NULL, NULL, (void**)&stmt);
#ifdef PREPARE2
        if (OCIStmtRelease(stmt, err, NULL, 0, OCI_DEFAULT) != OCI_SUCCESS) {
            rv = APR_EGENERAL;
        }
#else
        if (OCIHandleFree(stmt, OCI_HTYPE_STMT) != OCI_SUCCESS) {
            rv = APR_EGENERAL;
        }
#endif
    }
#ifdef PREPARE2
    if (OCIHandleFree(err, OCI_HTYPE_ERROR) != OCI_SUCCESS) {
        rv = APR_EGENERAL;
    }
#endif
    return rv;
}
#endif

static void dbd_oracle_init(apr_pool_t *pool)
{
    if (dbd_oracle_env == NULL) {
        /* Sadly, OCI_SHARED seems to be impossible to use, due to
         * various Oracle bugs.  See, for example, Oracle MetaLink bug 2972890
         * and PHP bug http://bugs.php.net/bug.php?id=23733
         */
        OCIEnvCreate(&dbd_oracle_env, OCI_THREADED, NULL,
                     NULL, NULL, NULL, 0, NULL);
    }
#ifdef GLOBAL_PREPARED_STATEMENTS
    if (oracle_statements == NULL) {
        
        oracle_statements = apr_hash_make(pool);
        apr_pool_cleanup_register(pool, oracle_statements,
                                  freeStatements, apr_pool_cleanup_null);
    }
#endif
}

static apr_dbd_t *dbd_oracle_open(apr_pool_t *pool, const char *params)
{
    apr_dbd_t *ret = apr_pcalloc(pool, sizeof(apr_dbd_t));
    int_errorcode;

    char *BLANK = "";
    struct {
        const char *field;
        char *value;
    } fields[] = {
        {"user", BLANK},
        {"pass", BLANK},
        {"dbname", BLANK},
        {"server", BLANK},
        {NULL, NULL}
    };
    int i;
    const char *ptr;
    const char *key;
    size_t klen;
    const char *value;
    size_t vlen;
    static const char *const delims = " \r\n\t;|,";

    ret->long_size = DEFAULT_LONG_SIZE;

    /* Use our own pool, to avoid possible race conditions
     * on pool ops
     */
    if (apr_pool_create(&ret->pool, pool) != APR_SUCCESS) {
        return NULL;
    }

    /* snitch parsing from the MySQL driver */
    for (ptr = strchr(params, '='); ptr; ptr = strchr(ptr, '=')) {
        for (key = ptr-1; isspace(*key); --key);
        klen = 0;
        while (isalpha(*key)) {
            if (key == params) {
                /* Don't parse off the front of the params */
                --key;
                ++klen;
                break;
            }
            --key;
            ++klen;
        }
        ++key;
        for (value = ptr+1; isspace(*value); ++value);
        vlen = strcspn(value, delims);
        for (i=0; fields[i].field != NULL; ++i) {
            if (!strncasecmp(fields[i].field, key, klen)) {
                fields[i].value = apr_pstrndup(pool, value, vlen);
                break;
            }
        }
        ptr = value+vlen;
    }

    ret->status = OCIHandleAlloc(dbd_oracle_env, (dvoid**)&ret->err,
                            OCI_HTYPE_ERROR, 0, NULL);
    switch (ret->status) {
    default:
#ifdef DEBUG
        printf("ret->status is %d\n", ret->status);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }

    ret->status = OCIHandleAlloc(dbd_oracle_env, (dvoid**)&ret->svr,
                            OCI_HTYPE_SERVER, 0, NULL);
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR %d (alloc svr): %s\n", ret->status, ret->buf);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }

    ret->status = OCIHandleAlloc(dbd_oracle_env, (dvoid**)&ret->svc,
                            OCI_HTYPE_SVCCTX, 0, NULL);
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR %d (alloc svc): %s\n", ret->status, ret->buf);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }

/* All the examples use the #else */
#if CAN_DO_LOGIN
    ret->status = OCILogon(dbd_oracle_env, ret->err, &ret->svc, fields[0].value,
                     strlen(fields[0].value), fields[1].value,
                     strlen(fields[1].value), fields[2].value,
                     strlen(fields[2].value));
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR: %s\n", ret->buf);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }
#else
    ret->status = OCIServerAttach(ret->svr, ret->err, fields[3].value,
                                  strlen(fields[3].value), OCI_DEFAULT);
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR %d (server attach): %s\n", ret->status, ret->buf);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }
    ret->status = OCIAttrSet(ret->svc, OCI_HTYPE_SVCCTX, ret->svr, 0,
                        OCI_ATTR_SERVER, ret->err);
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR %d (attr set): %s\n", ret->status, ret->buf);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }
    ret->status = OCIHandleAlloc(dbd_oracle_env, (dvoid**)&ret->auth,
                            OCI_HTYPE_SESSION, 0, NULL);
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR %d (alloc auth): %s\n", ret->status, ret->buf);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }
    ret->status = OCIAttrSet(ret->auth, OCI_HTYPE_SESSION, fields[0].value,
                        strlen(fields[0].value), OCI_ATTR_USERNAME, ret->err);
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR %d (attr username): %s\n", ret->status, ret->buf);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }
    ret->status = OCIAttrSet(ret->auth, OCI_HTYPE_SESSION, fields[1].value,
                        strlen(fields[1].value), OCI_ATTR_PASSWORD, ret->err);
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR %d (attr password): %s\n", ret->status, ret->buf);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }
    ret->status = OCISessionBegin(ret->svc, ret->err, ret->auth,
                             OCI_CRED_RDBMS, OCI_DEFAULT);
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR %d (session begin): %s\n", ret->status, ret->buf);
        break;
#else
        return NULL;
#endif
    case OCI_SUCCESS:
        break;
    }
    ret->status = OCIAttrSet(ret->svc, OCI_HTYPE_SVCCTX, ret->auth, 0,
                        OCI_ATTR_SESSION, ret->err);
    switch (ret->status) {
    default:
#ifdef DEBUG
        OCIErrorGet(ret->err, 1, NULL, &errorcode, ret->buf,
                    sizeof(ret->buf), OCI_HTYPE_ERROR);
        printf("OPEN ERROR %d (attr session): %s\n", ret->status, ret->buf);
#else
        return NULL;
#endif
        break;
    case OCI_SUCCESS:
        break;
    }
#endif
    return ret;
}

#ifdef EXPORT_NATIVE_FUNCS
static apr_size_t dbd_oracle_long_size_set(apr_dbd_t *sql,
                                           apr_size_t long_size)
{
    apr_size_t old_size = sql->long_size;
    sql->long_size = long_size;
    return old_size;
}
#endif

static const char *dbd_oracle_get_name(const apr_dbd_results_t *res, int n)
{
    define_arg *val = &res->statement->out[n];

    if ((n < 0) || (n >= res->statement->nout)) {
        return NULL;
    }
    return val->name;
}

static int dbd_oracle_get_row(apr_pool_t *pool, apr_dbd_results_t *res,
                              apr_dbd_row_t **rowp, int rownum)
{
    apr_dbd_row_t *row = *rowp;
    apr_dbd_t *sql = res->handle;
    int_errorcode;

    if (row == NULL) {
        row = apr_palloc(pool, sizeof(apr_dbd_row_t));
        *rowp = row;
        row->res = res;
        /* Oracle starts counting at 1 according to the docs */
        row->n = res->seek ? rownum : 1;
        row->pool = pool;
    }
    else {
        if (res->seek) {
            row->n = rownum;
        }
        else {
            ++row->n;
        }
    }

    if (res->seek) {
        sql->status = OCIStmtFetch2(res->statement->stmt, res->handle->err, 1,
                                    OCI_FETCH_ABSOLUTE, row->n, OCI_DEFAULT);
    }
    else {
        sql->status = OCIStmtFetch2(res->statement->stmt, res->handle->err, 1,
                                    OCI_FETCH_NEXT, 0, OCI_DEFAULT);
    }
    switch (sql->status) {
    case OCI_SUCCESS:
        (*rowp)->res = res;
        return 0;
    case OCI_NO_DATA:
        return -1;
    case OCI_ERROR:
#ifdef DEBUG
        OCIErrorGet(sql->err, 1, NULL, &errorcode,
                    sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR);
        printf("Execute error %d: %s\n", sql->status, sql->buf);
#endif
        /* fallthrough */
    default:
        return 1;
    }
    return 0;
}

static const char *dbd_oracle_error(apr_dbd_t *sql, int n)
{
    /* This is ugly.  Needs us to pass in a buffer of unknown size.
     * Either we put it on the handle, or we have to keep allocing/copying
     */
    sb4 errorcode;

    switch (sql->status) {
    case OCI_SUCCESS:
        return "OCI_SUCCESS";
    case OCI_SUCCESS_WITH_INFO:
        return "OCI_SUCCESS_WITH_INFO";
    case OCI_NEED_DATA:
        return "OCI_NEED_DATA";
    case OCI_NO_DATA:
        return "OCI_NO_DATA";
    case OCI_INVALID_HANDLE:
        return "OCI_INVALID_HANDLE";
    case OCI_STILL_EXECUTING:
        return "OCI_STILL_EXECUTING";
    case OCI_CONTINUE:
        return "OCI_CONTINUE";
    }

    switch (OCIErrorGet(sql->err, 1, NULL, &errorcode,
                        sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR)) {
    case OCI_SUCCESS:
        return sql->buf; 
    default:
        return "internal error: OCIErrorGet failed";
    }
}

static apr_status_t freeStatement(void *statement)
{
    int rv = APR_SUCCESS;
    OCIStmt *stmt = ((apr_dbd_prepared_t*)statement)->stmt;

#ifdef PREPARE2
    OCIError *err;

    if (OCIHandleAlloc(dbd_oracle_env, (dvoid**)&err, OCI_HTYPE_ERROR,
                       0, NULL) != OCI_SUCCESS) {
        return APR_EGENERAL;
    }
    if (OCIStmtRelease(stmt, err, NULL, 0, OCI_DEFAULT) != OCI_SUCCESS) {
        rv = APR_EGENERAL;
    }
    if (OCIHandleFree(err, OCI_HTYPE_ERROR) != OCI_SUCCESS) {
        rv = APR_EGENERAL;
    }
#else
    if (OCIHandleFree(stmt, OCI_HTYPE_STMT) != OCI_SUCCESS) {
        rv = APR_EGENERAL;
    }
#endif

    return rv;
}

static int dbd_oracle_select(apr_pool_t *pool, apr_dbd_t *sql,
                             apr_dbd_results_t **results,
                             const char *query, int seek)
{
    int ret = 0;
    apr_dbd_prepared_t *statement = NULL;

    ret = dbd_oracle_prepare(pool, sql, query, NULL, &statement);
    if (ret != 0) {
        return ret;
    }

    ret = dbd_oracle_pselect(pool, sql, results, statement, seek, 0, NULL);
    if (ret != 0) {
        return ret;
    }

    return ret;
}

static int dbd_oracle_query(apr_dbd_t *sql, int *nrows, const char *query)
{
    int ret = 0;
    apr_pool_t *pool;
    apr_dbd_prepared_t *statement = NULL;
    sword status;

    if (sql->trans && sql->trans->status == TRANS_ERROR) {
        return 1;
    }

    /* make our own pool so that APR allocations don't linger and so that
     * both Stmt and LOB handles are cleaned up (LOB handles may be
     * allocated when preparing APR_DBD_ORACLE_CLOB/BLOBs)
     */
    apr_pool_create(&pool, sql->pool);

    ret = dbd_oracle_prepare(pool, sql, query, NULL, &statement);
    if (ret == 0) {
        ret = dbd_oracle_pquery(pool, sql, nrows, statement, 0, NULL);
        if (ret == 0) {
            sql->status = OCIAttrGet(statement->stmt, OCI_HTYPE_STMT,
                                     nrows, 0, OCI_ATTR_ROW_COUNT,
                                     sql->err);
        }
    }

    apr_pool_destroy(pool);

    return ret;
}

static const char *dbd_oracle_escape(apr_pool_t *pool, const char *arg,
                                     apr_dbd_t *sql)
{
    return arg;        /* OCI has no concept of string escape */
}

static int dbd_oracle_prepare(apr_pool_t *pool, apr_dbd_t *sql,
                              const char *query, const char *label,
                              apr_dbd_prepared_t **statement)
{
    int ret = 0;
    size_t length;
    size_t bindlen = 0;
    int i;
    char *sqlptr;
    char *orastr;
    char *oraptr;
    apr_dbd_prepared_t *stmt ;

/* prepared statements in a global lookup table would be nice,
 * but we can't do that here because our pool may die leaving
 * the cached statement orphaned.
 * OTOH we can do that with Oracle statements, which aren't on
 * the pool,  so long as we don't register a cleanup on our pool!
 *
 * FIXME:
 * There's a race condition between cache-lookup and cache-set
 * But the worst outcome is a statement prepared more than once
 * and leaked.  Is that worth mutexing for?
 * Hmmm, yes it probably is ...  OK, done
 */

    if (*statement == NULL) {
        *statement = apr_pcalloc(pool, sizeof(apr_dbd_prepared_t));
    }
    stmt = *statement;
    stmt->handle = sql;
    stmt->pool = pool;

    /* If we have a label, we're going to cache it globally.
     * Check first if we already have it.  If not, prepare the
     * statement under mutex, so we don't end up leaking
     * concurrent statements
     *
     * FIXME: Oracle docs say a statement can be used even across
     * multiple servers, so I assume this is safe .....
     */
#ifdef GLOBAL_PREPARED_STATEMENTS
    if (label != NULL) {
        stmt->stmt = apr_hash_get(oracle_statements, label, APR_HASH_KEY_STRING);
        if (stmt->stmt != NULL) {
            return ret;
        }
        apr_dbd_mutex_lock();
        stmt->stmt = apr_hash_get(oracle_statements, label, APR_HASH_KEY_STRING);
        if (stmt->stmt != NULL) {
            apr_dbd_mutex_unlock();
            return ret;
        }
    }
#endif
    /* translate from apr_dbd to native query format */
    stmt->nargs = 0;
    for (sqlptr = (char*)query; *sqlptr; ++sqlptr) {
        if ((sqlptr[0] == '%') && isalnum(sqlptr[1])) {
            ++stmt->nargs;
        }
        else if ((sqlptr[0] == '%') && (sqlptr[1] == '%')) {
            /* ignore %% */
            ++sqlptr;
        }
    }
    length = strlen(query) + 1;
    for (i = stmt->nargs; i > 0; i /= 10) {
        ++bindlen;
    }
    length += (2 + bindlen) * stmt->nargs;      /* replace "%x" with ":aprN" */

    oraptr = orastr = apr_palloc(pool, length);

    if (stmt->nargs > 0) {
        stmt->args = apr_pcalloc(pool, stmt->nargs*sizeof(bind_arg));
        for (i=0; i<stmt->nargs; ++i) {
            stmt->args[i].type = APR_DBD_ORACLE_STRING;
        }
    }

    i = 0;
    for (sqlptr = (char*)query; *sqlptr; ++sqlptr) {
        if ((sqlptr[0] == '%') && isalnum(sqlptr[1])) {
            while (isdigit(*++sqlptr)) {
                stmt->args[i].len *= 10;
                stmt->args[i].len += (*sqlptr - '0');
            }
            oraptr += apr_snprintf(oraptr, length - (oraptr - orastr),
                                   ":apr%d", i + 1);
            switch (*sqlptr) {
            case 'd':
                stmt->args[i].type = APR_DBD_ORACLE_INT;
                break;
            case 'f':
                stmt->args[i].type = APR_DBD_ORACLE_FLOAT;
                break;
            case 'L':
                stmt->args[i].type = APR_DBD_ORACLE_LOB;
                break;
            /* BLOB and CLOB won't work - use LOB instead */
            case 'C':
                stmt->args[i].type = APR_DBD_ORACLE_CLOB;
                break;
            case 'B':
                stmt->args[i].type = APR_DBD_ORACLE_BLOB;
                break;
            /* default is STRING, but we already set that */
            default:
                stmt->args[i].type = APR_DBD_ORACLE_STRING;
                break;
            }
            ++i;
        }
        else if ((sqlptr[0] == '%') && (sqlptr[1] == '%')) {
            /* reduce %% to % */
            *oraptr++ = *sqlptr++;
        }
        else {
            *oraptr++ = *sqlptr;
        }
    }
    *oraptr = '\0';

    sql->status = OCIHandleAlloc(dbd_oracle_env, (dvoid**) &stmt->stmt,
                                 OCI_HTYPE_STMT, 0, NULL);
    switch (sql->status) {
    case OCI_SUCCESS:
        break;
    default:
        apr_dbd_mutex_unlock();
        return 1;
    }

    sql->status = OCIStmtPrepare(stmt->stmt, sql->err, orastr,
                                 strlen(orastr), OCI_NTV_SYNTAX, OCI_DEFAULT);
    switch (sql->status) {
    case OCI_SUCCESS:
        break;
    default:
        OCIHandleFree(stmt->stmt, OCI_HTYPE_STMT);
        apr_dbd_mutex_unlock();
        return 1;
    }

    apr_pool_cleanup_register(pool, stmt, freeStatement,
                              apr_pool_cleanup_null);

    /* Perl gets statement type here */
    sql->status = OCIAttrGet(stmt->stmt, OCI_HTYPE_STMT, &stmt->type, 0,
                             OCI_ATTR_STMT_TYPE, sql->err);
    switch (sql->status) {
    case OCI_SUCCESS:
        break;
    default:
        apr_dbd_mutex_unlock();
        return 1;
    }

/* Perl sets PREFETCH_MEMORY here, but the docs say there's a working default */
#if 0
    sql->status = OCIAttrSet(stmt->stmt, OCI_HTYPE_STMT, &prefetch_size,
                             sizeof(prefetch_size), OCI_ATTR_PREFETCH_MEMORY,
                             sql->err);
    switch (sql->status) {
    case OCI_SUCCESS:
        break;
    default:
        apr_dbd_mutex_unlock();
        return 1;
    }
#endif

    sql->status = OCI_SUCCESS;
    for (i = 0; i < stmt->nargs; ++i) {
        switch (stmt->args[i].type) {
        default:
        case APR_DBD_ORACLE_STRING:
            if (stmt->args[i].len == 0) {
                stmt->args[i].len = MAX_ARG_LEN;
            }
            stmt->args[i].value.stringval = apr_palloc(pool, stmt->args[i].len);
            sql->status = OCIBindByPos(stmt->stmt, &stmt->args[i].bind,
                                       sql->err, i+1,
                                       stmt->args[i].value.stringval,
                                       stmt->args[i].len,
                                       SQLT_STR,
                                       &stmt->args[i].ind,
                                       NULL,
                                       (ub2) 0, (ub4) 0,
                                       (ub4 *) 0, OCI_DEFAULT);
            break;
        case APR_DBD_ORACLE_FLOAT:
            stmt->args[i].value.raw = apr_palloc(pool, NUMERIC_FIELD_SIZE);
            sql->status = OCIBindByPos(stmt->stmt, &stmt->args[i].bind,
                                       sql->err, i+1,
                                       (void*)&stmt->args[i].value.floatval,
                                       sizeof(stmt->args[i].value.floatval),
                                       SQLT_FLT,
                                       &stmt->args[i].ind,
                                       NULL,
                                       (ub2) 0, (ub4) 0,
                                       (ub4 *) 0, OCI_DEFAULT);
            break;
        case APR_DBD_ORACLE_INT:
            stmt->args[i].value.raw = apr_palloc(pool, NUMERIC_FIELD_SIZE);
            sql->status = OCIBindByPos(stmt->stmt, &stmt->args[i].bind,
                                       sql->err, i+1,
                                       (void*)stmt->args[i].value.ival,
                                       sizeof(*stmt->args[i].value.ival),
                                       SQLT_INT,
                                       &stmt->args[i].ind,
                                       NULL,
                                       (ub2) 0, (ub4) 0,
                                       (ub4 *) 0, OCI_DEFAULT);
            break;
        /* lots of examples in the docs use this for LOB
         * but it relies on knowing the size in advance and
         * holding it in memory
         */
        case APR_DBD_ORACLE_LOB:
            break;        /* bind LOBs at write-time */
        /* This is also cited in the docs for LOB, if we don't
         * want the whole thing in memory
         */
        case APR_DBD_ORACLE_BLOB:
            sql->status = OCIDescriptorAlloc(dbd_oracle_env,
                                             (dvoid**)&stmt->args[i].value.lobval,
                                             OCI_DTYPE_LOB, 0, NULL);
            apr_pool_cleanup_register(pool, stmt->args[i].value.lobval,
                                      dbd_free_lobdesc,
                                      apr_pool_cleanup_null);
            sql->status = OCIBindByPos(stmt->stmt, &stmt->args[i].bind,
                                       sql->err, i+1,
                                       (void*) &stmt->args[i].value.lobval,
                                       -1,
                                       SQLT_BLOB,
                                       &stmt->args[i].ind,
                                       NULL,
                                       (ub2) 0, (ub4) 0,
                                       (ub4 *) 0, OCI_DEFAULT);
            break;
        /* This is also cited in the docs for LOB, if we don't
         * want the whole thing in memory
         */
        case APR_DBD_ORACLE_CLOB:
            sql->status = OCIDescriptorAlloc(dbd_oracle_env,
                                             (dvoid**)&stmt->args[i].value.lobval,
                                             OCI_DTYPE_LOB, 0, NULL);
            apr_pool_cleanup_register(pool, stmt->args[i].value.lobval,
                                      dbd_free_lobdesc,
                                      apr_pool_cleanup_null);
            sql->status = OCIBindByPos(stmt->stmt, &stmt->args[i].bind,
                                       sql->err, i+1,
                                       (dvoid*) &stmt->args[i].value.lobval,
                                       -1,
                                       SQLT_CLOB,
                                       &stmt->args[i].ind,
                                       NULL,
                                       (ub2) 0, (ub4) 0,
                                       (ub4 *) 0, OCI_DEFAULT);
            break;
        }
        switch (sql->status) {
        case OCI_SUCCESS:
            break;
        default:
            apr_dbd_mutex_unlock();
            return 1;
        }
    }
    switch (stmt->type) {
    case OCI_STMT_SELECT:
        ret = outputParams(sql, stmt);
        break;
    default:
        break;
    }
#ifdef GLOBAL_PREPARED_STATEMENTS
    if (label != NULL) {
        apr_hash_set(oracle_statements, label, APR_HASH_KEY_STRING, stmt->stmt);
        apr_dbd_mutex_unlock();
    }
#endif
    return ret;
}

static int outputParams(apr_dbd_t *sql, apr_dbd_prepared_t *stmt)
{
    OCIParam *parms;
    int i;
    int_errorcode;
    ub2 paramtype[DBD_ORACLE_MAX_COLUMNS];
    ub2 paramsize[DBD_ORACLE_MAX_COLUMNS];
    const char *paramname[DBD_ORACLE_MAX_COLUMNS];
    ub4 paramnamelen[DBD_ORACLE_MAX_COLUMNS];
    /* Perl uses 0 where we used 1 */
    sql->status = OCIStmtExecute(sql->svc, stmt->stmt, sql->err, 0, 0,
                                 NULL, NULL, OCI_DESCRIBE_ONLY);
    switch (sql->status) {
    case OCI_SUCCESS:
    case OCI_SUCCESS_WITH_INFO:
        break;
    case OCI_ERROR:
#ifdef DEBUG
        OCIErrorGet(sql->err, 1, NULL, &errorcode,
                    sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR);
        printf("Describing prepared statement: %s\n", sql->buf);
#endif
    default:
        return 1;
    }
    while (sql->status == OCI_SUCCESS) {
        sql->status = OCIParamGet(stmt->stmt, OCI_HTYPE_STMT,
                                  sql->err, (dvoid**)&parms, stmt->nout+1);
        switch (sql->status) {
        case OCI_SUCCESS:
            sql->status = OCIAttrGet(parms, OCI_DTYPE_PARAM,
                                     &paramtype[stmt->nout],
                                     0, OCI_ATTR_DATA_TYPE, sql->err);
            sql->status = OCIAttrGet(parms, OCI_DTYPE_PARAM,
                                     &paramsize[stmt->nout],
                                     0, OCI_ATTR_DATA_SIZE, sql->err);
            sql->status = OCIAttrGet(parms, OCI_DTYPE_PARAM,
                                     &paramname[stmt->nout],
                                     &paramnamelen[stmt->nout],
                                     OCI_ATTR_NAME, sql->err);
            ++stmt->nout;
        }
    }
    switch (sql->status) {
    case OCI_SUCCESS:
        break;
    case OCI_ERROR:
        break;        /* this is what we expect at end-of-loop */
    default:
        return 1;
    }

    /* OK, the above works.  We have the params; now OCIDefine them */
    stmt->out = apr_palloc(stmt->pool, stmt->nout*sizeof(define_arg));
    for (i=0; i<stmt->nout; ++i) {
        stmt->out[i].type = paramtype[i];
        stmt->out[i].len = stmt->out[i].sz = paramsize[i];
        stmt->out[i].name = apr_pstrmemdup(stmt->pool,
                                           paramname[i], paramnamelen[i]);
        switch (stmt->out[i].type) {
        default:
            switch (stmt->out[i].type) {
            case SQLT_NUM:           /* 2: numeric, Perl worst case=130+38+3 */
                stmt->out[i].sz = 171;
                break;
            case SQLT_CHR:           /* 1: char */
            case SQLT_AFC:           /* 96: ANSI fixed char */
                stmt->out[i].sz *= 4; /* ugh, wasteful UCS-4 handling */
                break;
            case SQLT_DAT:           /* 12: date, depends on NLS date format */
                stmt->out[i].sz = 75;
                break;
            case SQLT_BIN:           /* 23: raw binary, perhaps UTF-16? */
                stmt->out[i].sz *= 2;
                break;
            case SQLT_RID:           /* 11: rowid */
            case SQLT_RDD:           /* 104: rowid descriptor */
                stmt->out[i].sz = 20;
                break;
            case SQLT_TIMESTAMP:     /* 187: timestamp */
            case SQLT_TIMESTAMP_TZ:  /* 188: timestamp with time zone */
            case SQLT_INTERVAL_YM:   /* 189: interval year-to-month */
            case SQLT_INTERVAL_DS:   /* 190: interval day-to-second */
            case SQLT_TIMESTAMP_LTZ: /* 232: timestamp with local time zone */
                stmt->out[i].sz = 75;
                break;
            default:
#ifdef DEBUG
                printf("Unsupported data type: %d\n", stmt->out[i].type);
#endif
                break;
            }
            ++stmt->out[i].sz;
            stmt->out[i].buf.raw = apr_palloc(stmt->pool, stmt->out[i].sz);
            sql->status = OCIDefineByPos(stmt->stmt, &stmt->out[i].defn,
                                         sql->err, i+1,
                                         stmt->out[i].buf.stringval,
                                         stmt->out[i].sz, SQLT_STR,
                                         &stmt->out[i].ind, &stmt->out[i].len,
                                         0, OCI_DEFAULT);
            break;
        case SQLT_LNG: /* 8: long */
            stmt->out[i].sz = sql->long_size * 4 + 4; /* ugh, UCS-4 handling */
            stmt->out[i].buf.raw = apr_palloc(stmt->pool, stmt->out[i].sz);
            sql->status = OCIDefineByPos(stmt->stmt, &stmt->out[i].defn,
                                         sql->err, i+1,
                                         stmt->out[i].buf.raw,
                                         stmt->out[i].sz, SQLT_LVC,
                                         &stmt->out[i].ind, NULL,
                                         0, OCI_DEFAULT);
            break;
        case SQLT_LBI: /* 24: long binary, perhaps UTF-16? */
            stmt->out[i].sz = sql->long_size * 2 + 4; /* room for int prefix */
            stmt->out[i].buf.raw = apr_palloc(stmt->pool, stmt->out[i].sz);
            sql->status = OCIDefineByPos(stmt->stmt, &stmt->out[i].defn,
                                         sql->err, i+1,
                                         stmt->out[i].buf.raw,
                                         stmt->out[i].sz, SQLT_LVB,
                                         &stmt->out[i].ind, NULL,
                                         0, OCI_DEFAULT);
            break;
        case SQLT_BLOB: /* 113 */
        case SQLT_CLOB: /* 112 */
/*http://download-west.oracle.com/docs/cd/B10501_01/appdev.920/a96584/oci05bnd.htm#434937*/
            sql->status = OCIDescriptorAlloc(dbd_oracle_env,
                                             (dvoid**)&stmt->out[i].buf.lobval,
                                             OCI_DTYPE_LOB, 0, NULL);
            apr_pool_cleanup_register(stmt->pool, stmt->out[i].buf.lobval,
                                      dbd_free_lobdesc,
                                      apr_pool_cleanup_null);
            sql->status = OCIDefineByPos(stmt->stmt, &stmt->out[i].defn,
                                         sql->err, i+1,
                                         (dvoid*) &stmt->out[i].buf.lobval,
                                         -1, stmt->out[i].type,
                                         &stmt->out[i].ind, &stmt->out[i].len,
                                         0, OCI_DEFAULT);
            break;
        }
        switch (sql->status) {
        case OCI_SUCCESS:
            break;
        default:
            return 1;
        }
    }
    return 0;
}

static int dbd_oracle_pvquery(apr_pool_t *pool, apr_dbd_t *sql,
                              int *nrows, apr_dbd_prepared_t *statement,
                              va_list args)
{
    const char *arg = NULL;
    OCISnapshot *oldsnapshot = NULL;
    OCISnapshot *newsnapshot = NULL;
    apr_dbd_transaction_t* trans = sql->trans;
    int i;
    int exec_mode;
    int_errorcode;

    if (trans) {
        switch (trans->status) {
        case TRANS_ERROR:
            return -1;
        case TRANS_NONE:
            trans = NULL;
            break;
        case TRANS_1:
            oldsnapshot = trans->snapshot1;
            newsnapshot = trans->snapshot2;
            trans->status = TRANS_2;
            break;
        case TRANS_2:
            oldsnapshot = trans->snapshot2;
            newsnapshot = trans->snapshot1;
            trans->status = TRANS_1;
            break;
        }
        exec_mode = OCI_DEFAULT;
    }
    else {
        exec_mode = OCI_COMMIT_ON_SUCCESS;
    }

    /* we've bound these vars, so now we just copy data in to them */
    for (i=0; i<statement->nargs; ++i) {
        switch (statement->args[i].type) {
        case APR_DBD_ORACLE_INT:
            arg = va_arg(args, char*);
            sscanf(arg, "%d", statement->args[i].value.ival);
            break;
        case APR_DBD_ORACLE_FLOAT:
            arg = va_arg(args, char*);
            sscanf(arg, "%lf", statement->args[i].value.floatval);
            break;
        case APR_DBD_ORACLE_BLOB:
        case APR_DBD_ORACLE_CLOB:
            sql->status = OCIAttrSet(statement->args[i].value.lobval,
                                     OCI_DTYPE_LOB, &null, 0,
                                     OCI_ATTR_LOBEMPTY, sql->err);
            break;
        case APR_DBD_ORACLE_LOB:
            /* requires strlen() over large data, which may fail for binary */
            statement->args[i].value.raw = va_arg(args, char*);
            statement->args[i].len =
                strlen(statement->args[i].value.stringval);
            sql->status = OCIBindByPos(statement->stmt,
                                       &statement->args[i].bind,
                                       sql->err, i+1,
                                       (void*)statement->args[i].value.raw,
                                       statement->args[i].len, SQLT_LNG,
                                       &statement->args[i].ind,
                                       NULL,
                                       (ub2) 0, (ub4) 0,
                                       (ub4 *) 0, OCI_DEFAULT);
            break;
        case APR_DBD_ORACLE_STRING:
        default:
            arg = va_arg(args, char*);
            if (strlen(arg) >= statement->args[i].len) {
                strncpy(statement->args[i].value.stringval, arg,
                        statement->args[i].len-1);
            }
            else {
                strcpy(statement->args[i].value.stringval, arg);
            }
            break;
        }
    }

    sql->status = OCIStmtExecute(sql->svc, statement->stmt, sql->err, 1, 0,
                                 oldsnapshot, newsnapshot, exec_mode);
    switch (sql->status) {
    case OCI_SUCCESS:
        break;
    case OCI_ERROR:
#ifdef DEBUG
        OCIErrorGet(sql->err, 1, NULL, &errorcode,
                    sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR);
        printf("Execute error %d: %s\n", sql->status, sql->buf);
#endif
        /* fallthrough */
    default:
        if (TXN_NOTICE_ERRORS(trans)) {
            trans->status = TRANS_ERROR;
        }
        return 1;
    }

    sql->status = OCIAttrGet(statement->stmt, OCI_HTYPE_STMT, nrows, 0,
                             OCI_ATTR_ROW_COUNT, sql->err);
    return 0;
}

static int dbd_oracle_pquery(apr_pool_t *pool, apr_dbd_t *sql,
                             int *nrows, apr_dbd_prepared_t *statement,
                             int nargs, const char **values)
{
    int_errorcode;
    OCISnapshot *oldsnapshot = NULL;
    OCISnapshot *newsnapshot = NULL;
    apr_dbd_transaction_t* trans = sql->trans;
    int i;
    int exec_mode;

    if (trans) {
        switch (trans->status) {
        case TRANS_ERROR:
            return -1;
        case TRANS_NONE:
            trans = NULL;
            break;
        case TRANS_1:
            oldsnapshot = trans->snapshot1;
            newsnapshot = trans->snapshot2;
            trans->status = TRANS_2;
            break;
        case TRANS_2:
            oldsnapshot = trans->snapshot2;
            newsnapshot = trans->snapshot1;
            trans->status = TRANS_1;
            break;
        }
        exec_mode = OCI_DEFAULT;
    }
    else {
        exec_mode = OCI_COMMIT_ON_SUCCESS;
    }

    /* we've bound these vars, so now we just copy data in to them */
    if (nargs > statement->nargs) {
        nargs = statement->nargs;
    }
    for (i=0; i<nargs; ++i) {
        switch (statement->args[i].type) {
        case APR_DBD_ORACLE_INT:
            sscanf(values[i], "%d", statement->args[i].value.ival);
            break;
        case APR_DBD_ORACLE_FLOAT:
            sscanf(values[i], "%lf", statement->args[i].value.floatval);
            break;
        case APR_DBD_ORACLE_BLOB:
        case APR_DBD_ORACLE_CLOB:
            sql->status = OCIAttrSet(statement->args[i].value.lobval,
                                     OCI_DTYPE_LOB, &null, 0,
                                     OCI_ATTR_LOBEMPTY, sql->err);
            break;
        case APR_DBD_ORACLE_LOB:
            /* requires strlen() over large data, which may fail for binary */
            statement->args[i].value.raw = (char*)values[i];
            statement->args[i].len =
                strlen(statement->args[i].value.stringval);
            sql->status = OCIBindByPos(statement->stmt,
                                       &statement->args[i].bind,
                                       sql->err, i+1,
                                       (void*)statement->args[i].value.raw,
                                       statement->args[i].len, SQLT_LNG,
                                       &statement->args[i].ind,
                                       NULL,
                                       (ub2) 0, (ub4) 0,
                                       (ub4 *) 0, OCI_DEFAULT);
            break;
        case APR_DBD_ORACLE_STRING:
        default:
            if (strlen(values[i]) >= statement->args[i].len) {
                strncpy(statement->args[i].value.stringval, values[i],
                        statement->args[i].len-1);
            }
            else {
                strcpy(statement->args[i].value.stringval, values[i]);
            }
            break;
        }
    }

    sql->status = OCIStmtExecute(sql->svc, statement->stmt, sql->err, 1, 0,
                                 oldsnapshot, newsnapshot, exec_mode);
    switch (sql->status) {
    case OCI_SUCCESS:
        break;
    case OCI_ERROR:
#ifdef DEBUG
        OCIErrorGet(sql->err, 1, NULL, &errorcode,
                    sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR);
        printf("Execute error %d: %s\n", sql->status, sql->buf);
#endif
        /* fallthrough */
    default:
        if (TXN_NOTICE_ERRORS(trans)) {
            trans->status = TRANS_ERROR;
        }
        return 1;
    }

    sql->status = OCIAttrGet(statement->stmt, OCI_HTYPE_STMT, nrows, 0,
                             OCI_ATTR_ROW_COUNT, sql->err);
    return 0;
}

static int dbd_oracle_pvselect(apr_pool_t *pool, apr_dbd_t *sql,
                               apr_dbd_results_t **results,
                               apr_dbd_prepared_t *statement,
                               int seek, va_list args)
{
    int i;
    char *arg;
    int exec_mode = seek ? OCI_STMT_SCROLLABLE_READONLY : OCI_DEFAULT;
    OCISnapshot *oldsnapshot = NULL;
    OCISnapshot *newsnapshot = NULL;
    apr_dbd_transaction_t* trans = sql->trans;
    int_errorcode;

    if (trans) {
        switch (trans->status) {
        case TRANS_ERROR:
            return 1;
        case TRANS_NONE:
            trans = NULL;
            break;
        case TRANS_1:
            oldsnapshot = trans->snapshot1;
            newsnapshot = trans->snapshot2;
            trans->status = TRANS_2;
            break;
        case TRANS_2:
            oldsnapshot = trans->snapshot2;
            newsnapshot = trans->snapshot1;
            trans->status = TRANS_1;
            break;
        }
    }

    /* we've bound these vars, so now we just copy data in to them */
    for (i=0; i<statement->nargs; ++i) {
        int len;
        switch (statement->args[i].type) {
        case APR_DBD_ORACLE_INT:
            arg = va_arg(args, char*);
            sscanf(arg, "%d", statement->args[i].value.ival);
            break;
        case APR_DBD_ORACLE_FLOAT:
            arg = va_arg(args, char*);
            sscanf(arg, "%lf", statement->args[i].value.floatval);
            break;
        case APR_DBD_ORACLE_BLOB:
        case APR_DBD_ORACLE_CLOB:
            sql->status = OCIAttrSet(statement->args[i].value.lobval,
                                     OCI_DTYPE_LOB, &null, 0,
                                     OCI_ATTR_LOBEMPTY, sql->err);
            break;
        case APR_DBD_ORACLE_STRING:
        default:
            arg = va_arg(args, char*);
            len = strlen(arg);
            if (len >= statement->args[i].len) {
                len = statement->args[i].len - 1;
                strncpy(statement->args[i].value.stringval, arg, len);
                statement->args[i].value.stringval[len] = '\0';
            }
            else {
                strcpy(statement->args[i].value.stringval, arg);
            }
            ++len;
            sql->status = OCIAttrSet(statement->args[i].bind,
                                     OCI_DTYPE_PARAM, &len, 0,
                                     OCI_ATTR_DATA_SIZE, sql->err);
            break;
        }
    }

    sql->status = OCIStmtExecute(sql->svc, statement->stmt, sql->err, 0, 0,
                                 oldsnapshot, newsnapshot, exec_mode);
    switch (sql->status) {
    case OCI_SUCCESS:
        break;
    case OCI_ERROR:
#ifdef DEBUG
        OCIErrorGet(sql->err, 1, NULL, &errorcode,
                    sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR);
        printf("Executing prepared statement: %s\n", sql->buf);
#endif
        /* fallthrough */
    default:
        if (TXN_NOTICE_ERRORS(trans)) {
            trans->status = TRANS_ERROR;
        }
        return 1;
    }

    if (!*results) {
        *results = apr_palloc(pool, sizeof(apr_dbd_results_t));
    }
    (*results)->handle = sql;
    (*results)->statement = statement;
    (*results)->seek = seek;
    (*results)->rownum = seek ? 0 : -1;

    return 0;
}

static int dbd_oracle_pselect(apr_pool_t *pool, apr_dbd_t *sql,
                              apr_dbd_results_t **results,
                              apr_dbd_prepared_t *statement,
                              int seek, int nargs, const char **values)
{
    int i;
    int exec_mode = seek ? OCI_STMT_SCROLLABLE_READONLY : OCI_DEFAULT;
    OCISnapshot *oldsnapshot = NULL;
    OCISnapshot *newsnapshot = NULL;
    apr_dbd_transaction_t* trans = sql->trans;

    if (trans) {
        switch (trans->status) {
        case TRANS_ERROR:
            return 1;
        case TRANS_NONE:
            trans = NULL;
            break;
        case TRANS_1:
            oldsnapshot = trans->snapshot1;
            newsnapshot = trans->snapshot2;
            trans->status = TRANS_2;
            break;
        case TRANS_2:
            oldsnapshot = trans->snapshot2;
            newsnapshot = trans->snapshot1;
            trans->status = TRANS_1;
            break;
        }
    }

    /* we've bound these vars, so now we just copy data in to them */
    if (nargs > statement->nargs) {
        nargs = statement->nargs;
    }
    for (i=0; i<nargs; ++i) {
        switch (statement->args[i].type) {
        case APR_DBD_ORACLE_INT:
            sscanf(values[i], "%d", statement->args[i].value.ival);
            break;
        case APR_DBD_ORACLE_FLOAT:
            sscanf(values[i], "%lf", statement->args[i].value.floatval);
            break;
        case APR_DBD_ORACLE_BLOB:
        case APR_DBD_ORACLE_CLOB:
            sql->status = OCIAttrSet(statement->args[i].value.lobval,
                                     OCI_DTYPE_LOB, &null, 0,
                                     OCI_ATTR_LOBEMPTY, sql->err);
            break;
        case APR_DBD_ORACLE_STRING:
        default:
            if (strlen(values[i]) >= MAX_ARG_LEN) {
                strncpy(statement->args[i].value.stringval, values[i],
                        MAX_ARG_LEN-1);
            }
            else {
                strcpy(statement->args[i].value.stringval, values[i]);
            }
            break;
        }
    }

    sql->status = OCIStmtExecute(sql->svc, statement->stmt, sql->err, 0, 0,
                                 oldsnapshot, newsnapshot, exec_mode);
    switch (sql->status) {
    int_errorcode;
    case OCI_SUCCESS:
        break;
    case OCI_ERROR:
#ifdef DEBUG
        OCIErrorGet(sql->err, 1, NULL, &errorcode,
                    sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR);
        printf("Executing prepared statement: %s\n", sql->buf);
#endif
        /* fallthrough */
    default:
        if (TXN_NOTICE_ERRORS(trans)) {
            trans->status = TRANS_ERROR;
        }
        return 1;
    }

    if (!*results) {
        *results = apr_palloc(pool, sizeof(apr_dbd_results_t));
    }
    (*results)->handle = sql;
    (*results)->statement = statement;
    (*results)->seek = seek;
    (*results)->rownum = seek ? 0 : -1;

    return 0;
}

static int dbd_oracle_start_transaction(apr_pool_t *pool, apr_dbd_t *sql,
                                        apr_dbd_transaction_t **trans)
{
    int ret = 0;
    int_errorcode;
    if (*trans) {
        dbd_oracle_end_transaction(*trans);
    }
    else {
        *trans = apr_pcalloc(pool, sizeof(apr_dbd_transaction_t));
        OCIHandleAlloc(dbd_oracle_env, (dvoid**)&(*trans)->trans,
                       OCI_HTYPE_TRANS, 0, 0);
        OCIAttrSet(sql->svc, OCI_HTYPE_SVCCTX, (*trans)->trans, 0,
                   OCI_ATTR_TRANS, sql->err);
    }


    sql->status = OCITransStart(sql->svc, sql->err, TRANS_TIMEOUT,
                                OCI_TRANS_NEW);
    switch (sql->status) {
    case OCI_ERROR:
#ifdef DEBUG
        OCIErrorGet(sql->err, 1, NULL, &errorcode, sql->buf,
                    sizeof(sql->buf), OCI_HTYPE_ERROR);
        printf("Transaction: %s\n", sql->buf);
#endif
        ret = 1;
        break;
    case OCI_SUCCESS:
        (*trans)->handle = sql;
        (*trans)->status = TRANS_1;
        sql->trans = *trans;
        switch (OCIDescriptorAlloc(dbd_oracle_env,
                                   (dvoid**)&(*trans)->snapshot1,
                                   OCI_DTYPE_SNAP, 0, NULL)) {
        case OCI_SUCCESS:
            apr_pool_cleanup_register(pool, (*trans)->snapshot1,
                                      dbd_free_snapshot, apr_pool_cleanup_null);
            break;
        case OCI_INVALID_HANDLE:
            ret = 1;
            break;
        }
        switch (OCIDescriptorAlloc(dbd_oracle_env,
                                   (dvoid**)&(*trans)->snapshot2,
                                   OCI_DTYPE_SNAP, 0, NULL)) {
        case OCI_SUCCESS:
            apr_pool_cleanup_register(pool, (*trans)->snapshot2,
                                      dbd_free_snapshot, apr_pool_cleanup_null);
            break;
        case OCI_INVALID_HANDLE:
            ret = 1;
            break;
        }
        break;
    default:
        ret = 1;
        break;
    }
    return ret;
}

static int dbd_oracle_end_transaction(apr_dbd_transaction_t *trans)
{
    int ret = 1;             /* no transaction is an error cond */
    sword status;
    apr_dbd_t *handle = trans->handle;
    if (trans) {
        switch (trans->status) {
        case TRANS_NONE:     /* No trans is an error here */
            status = OCI_ERROR;
            break;
        case TRANS_ERROR:
            status = OCITransRollback(handle->svc, handle->err, OCI_DEFAULT);
            break;
        default:
            /* rollback on explicit rollback request */
            if (TXN_DO_ROLLBACK(trans)) {
                status = OCITransRollback(handle->svc, handle->err, OCI_DEFAULT);
            } else {
                status = OCITransCommit(handle->svc, handle->err, OCI_DEFAULT);
            }
            break;
        }

        handle->trans = NULL;

        switch (status) {
        case OCI_SUCCESS:
            ret = 0;
            break;
        default:
            ret = 3;
            break;
        }
    }
    return ret;
}

static int dbd_oracle_transaction_mode_get(apr_dbd_transaction_t *trans)
{
    if (!trans)
        return APR_DBD_TRANSACTION_COMMIT;

    return trans->mode;
}

static int dbd_oracle_transaction_mode_set(apr_dbd_transaction_t *trans,
                                           int mode)
{
    if (!trans)
        return APR_DBD_TRANSACTION_COMMIT;

    return trans->mode = (mode & TXN_MODE_BITS);
}

/* This doesn't work for BLOB because of NULLs, but it can fake it
 * if the BLOB is really a string
 */
static const char *dbd_oracle_get_entry(const apr_dbd_row_t *row, int n)
{
    int_errorcode;
    ub4 len = 0;
    ub1 csform = 0;
    ub2 csid = 0;
    apr_size_t buflen = 0;
    char *buf = NULL;
    define_arg *val = &row->res->statement->out[n];
    apr_dbd_t *sql = row->res->handle;

    if ((n < 0) || (n >= row->res->statement->nout) || (val->ind == -1)) {
        return NULL;
    }

    switch (val->type) {
    case SQLT_BLOB:
    case SQLT_CLOB:
        sql->status = OCILobGetLength(sql->svc, sql->err, val->buf.lobval,
                                      &len);
        switch (sql->status) {
        case OCI_SUCCESS:
        case OCI_SUCCESS_WITH_INFO:
            if (len == 0) {
                buf = "";
            }
            break;
        case OCI_ERROR:
#ifdef DEBUG
            OCIErrorGet(sql->err, 1, NULL, &errorcode,
                        sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR);
            printf("Finding LOB length: %s\n", sql->buf);
            break;
#endif
        default:
            break;
        }

        if (len == 0) {
            break;
        }

        if (val->type == APR_DBD_ORACLE_CLOB) {

#if 1
            /* Is this necessary, or can it be defaulted? */
            sql->status = OCILobCharSetForm(dbd_oracle_env, sql->err,
                                            val->buf.lobval, &csform);
            if (sql->status == OCI_SUCCESS) {
                sql->status = OCILobCharSetId(dbd_oracle_env, sql->err,
                                              val->buf.lobval, &csid);
            }
            switch (sql->status) {
            case OCI_SUCCESS:
            case OCI_SUCCESS_WITH_INFO:
                buflen = (len+1) * 4; /* ugh, wasteful UCS-4 handling */
                /* zeroise all - where the string ends depends on charset */
                buf = apr_pcalloc(row->pool, buflen);
                break;
#ifdef DEBUG
            case OCI_ERROR:
                OCIErrorGet(sql->err, 1, NULL, &errorcode,
                            sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR);
                printf("Reading LOB character set: %s\n", sql->buf);
                break; /*** XXX?? ***/
#endif
            default:
                break; /*** XXX?? ***/
            }
#else   /* ignore charset */
            buflen = (len+1) * 4; /* ugh, wasteful UCS-4 handling */
            /* zeroise all - where the string ends depends on charset */
            buf = apr_pcalloc(row->pool, buflen);
#endif
        } else {
            /* BUG: this'll only work if the BLOB looks like a string */
            buflen = len;
            buf = apr_palloc(row->pool, buflen+1);
            buf[buflen] = 0;
        }

        if (!buf) {
            break;
        }

        sql->status = OCILobRead(sql->svc, sql->err, val->buf.lobval,
                                 &len, 1, (dvoid*) buf, buflen,
                                 NULL, NULL, csid, csform);
        switch (sql->status) {
        case OCI_SUCCESS:
        case OCI_SUCCESS_WITH_INFO:
            break;
#ifdef DEBUG
        case OCI_ERROR:
            OCIErrorGet(sql->err, 1, NULL, &errorcode,
                        sql->buf, sizeof(sql->buf), OCI_HTYPE_ERROR);
            printf("Reading LOB: %s\n", sql->buf);
            buf = NULL; /*** XXX?? ***/
            break;
#endif
        default:
            buf = NULL; /*** XXX?? ***/
            break;
        }

        break;
    case SQLT_LNG:
    case SQLT_LBI:
        /* raw is struct { ub4 len; char *buf; } */
        len = *(ub4*) val->buf.raw;
        buf = apr_pstrndup(row->pool, val->buf.stringval + sizeof(ub4), len);
        break;
    default:
        buf = apr_pstrndup(row->pool, val->buf.stringval, val->len);
        break;
    }
    return (const char*) buf;
}

static apr_status_t dbd_oracle_close(apr_dbd_t *handle)
{
    /* FIXME: none of the oracle docs/examples say anything about
     * closing/releasing handles.  Which seems unlikely ...
     */

    /* OK, let's grab from cdemo again.
     * cdemo81 does nothing; cdemo82 does OCIHandleFree on the handles
     */
    switch (OCISessionEnd(handle->svc, handle->err, handle->auth,
            (ub4)OCI_DEFAULT)) {
    default:
        break;
    }
    switch (OCIServerDetach(handle->svr, handle->err, (ub4) OCI_DEFAULT )) {
    default:
        break;
    }
    /* does OCISessionEnd imply this? */
    switch (OCIHandleFree((dvoid *) handle->auth, (ub4) OCI_HTYPE_SESSION)) {
    default:
        break;
    }
    switch (OCIHandleFree((dvoid *) handle->svr, (ub4) OCI_HTYPE_SERVER)) {
    default:
        break;
    }
    switch (OCIHandleFree((dvoid *) handle->svc, (ub4) OCI_HTYPE_SVCCTX)) {
    default:
        break;
    }
    switch (OCIHandleFree((dvoid *) handle->err, (ub4) OCI_HTYPE_ERROR)) {
    default:
        break;
    }
    apr_pool_destroy(handle->pool);
    return APR_SUCCESS;
}

static apr_status_t dbd_oracle_check_conn(apr_pool_t *pool,
                                          apr_dbd_t *handle)
{
    /* FIXME: need to find this in the docs */
    return APR_ENOTIMPL;
}

static int dbd_oracle_select_db(apr_pool_t *pool, apr_dbd_t *handle,
                                const char *name)
{
    /* FIXME: need to find this in the docs */
    return APR_ENOTIMPL;
}

static void *dbd_oracle_native(apr_dbd_t *handle)
{
    /* FIXME: can we do anything better?  Oracle doesn't seem to have
     * a concept of a handle in the sense we use it.
     */
    return dbd_oracle_env;
}

static int dbd_oracle_num_cols(apr_dbd_results_t* res)
{
    return res->statement->nout;
}

static int dbd_oracle_num_tuples(apr_dbd_results_t* res)
{
    if (!res->seek) {
        return -1;
    }
    if (res->nrows >= 0) {
        return res->nrows;
    }
    res->handle->status = OCIAttrGet(res->statement->stmt, OCI_HTYPE_STMT,
                                     &res->nrows, 0, OCI_ATTR_ROW_COUNT,
                                     res->handle->err);
    return res->nrows;
}

APU_DECLARE_DATA const apr_dbd_driver_t apr_dbd_oracle_driver = {
    "oracle",
    dbd_oracle_init,
    dbd_oracle_native,
    dbd_oracle_open,
    dbd_oracle_check_conn,
    dbd_oracle_close,
    dbd_oracle_select_db,
    dbd_oracle_start_transaction,
    dbd_oracle_end_transaction,
    dbd_oracle_query,
    dbd_oracle_select,
    dbd_oracle_num_cols,
    dbd_oracle_num_tuples,
    dbd_oracle_get_row,
    dbd_oracle_get_entry,
    dbd_oracle_error,
    dbd_oracle_escape,
    dbd_oracle_prepare,
    dbd_oracle_pvquery,
    dbd_oracle_pvselect,
    dbd_oracle_pquery,
    dbd_oracle_pselect,
    dbd_oracle_get_name,
    dbd_oracle_transaction_mode_get,
    dbd_oracle_transaction_mode_set
};
#endif
