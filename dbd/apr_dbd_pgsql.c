/* Copyright 2000-2005 The Apache Software Foundation
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

#if APU_HAVE_PGSQL

#include <ctype.h>
#include <stdlib.h>

#include <libpq-fe.h>

#include "apr_strings.h"

#define QUERY_MAX_ARGS 40

typedef PGconn apr_dbd_t;
typedef struct {
    int errnum;
    apr_dbd_t *handle;
} apr_dbd_transaction;

typedef struct {
    int random;
    PGconn *handle;
    PGresult *res;
    size_t ntuples;
    size_t sz;
    size_t index;
} apr_dbd_results;
typedef struct {
    int n;
    apr_dbd_results *res;
} apr_dbd_row;
typedef struct apr_dbd_prepared {
    const char *name;
    int prepared;
} apr_dbd_prepared;

#define dbd_pgsql_is_success(x) (((x)==PGRES_EMPTY_QUERY)||((x)==PGRES_COMMAND_OK)||((x)==PGRES_TUPLES_OK))

#define APR_DBD_INTERNAL
#include "apr_dbd.h"

static int dbd_pgsql_select(apr_pool_t *pool, apr_dbd_t *sql,
                            apr_dbd_transaction *trans,
                            apr_dbd_results **results,
                            const char *query, int seek)
{
    PGresult *res;
    int ret;
    if ( trans && trans->errnum ) {
        return trans->errnum;
    }
    if (seek) { /* synchronous query */
        res = PQexec(sql, query);
        if (res) {
            ret = PQresultStatus(res);
            if (dbd_pgsql_is_success(ret)) {
                ret = 0;
            } else {
                PQclear(res);
            }
        } else {
            ret = PGRES_FATAL_ERROR;
        }
        if (ret != 0) {
            if (trans) {
                trans->errnum = ret;
            }
            return ret;
        }
        if (!*results) {
            *results = apr_pcalloc(pool, sizeof(apr_dbd_results));
        }
        (*results)->res = res;
        (*results)->ntuples = PQntuples(res);
        (*results)->sz = PQnfields(res);
        (*results)->random = seek;
        apr_pool_cleanup_register(pool, res, (void*)PQclear,
                                  apr_pool_cleanup_null);
    }
    else {
        if (PQsendQuery(sql, query) == 0) {
            if (trans) {
                trans->errnum = 1;
            }
            return 1;
        }
        if (*results == NULL) {
            *results = apr_pcalloc(pool, sizeof(apr_dbd_results));
        }
        (*results)->random = seek;
        (*results)->handle = sql;
    }
    return 0;
}
static int dbd_pgsql_get_row(apr_pool_t *pool, apr_dbd_results *res,
                             apr_dbd_row **rowp, int rownum)
{
    apr_dbd_row *row = *rowp;
    int sequential = ((rownum >= 0) && res->random) ? 0 : 1;

    if (row == NULL) {
        row = apr_palloc(pool, sizeof(apr_dbd_row));
        *rowp = row;
        row->res = res;
        row->n = sequential ? 0 : rownum;
    }
    else {
        if ( sequential ) {
            ++row->n;
        }
        else {
            row->n = rownum;
        }
    }

    if (res->random) {
        if (row->n >= res->ntuples) {
            *rowp = NULL;
            apr_pool_cleanup_kill(pool, res->res, (void*)PQclear);
            PQclear(res->res);
            res->res = NULL;
            return -1;
        }
    }
    else {
        if (row->n >= res->ntuples) {
            /* no data; we have to fetch some */
            row->n -= res->ntuples;
            if (res->res != NULL) {
                PQclear(res->res);
            }
            res->res = PQgetResult(res->handle);
            if (res->res) {
                res->ntuples = PQntuples(res->res);
                while (res->ntuples == 0) {
                  /* if we got an empty result, clear it, wait a mo, try again */
                    PQclear(res->res);
                    apr_sleep(100000);        /* 0.1 secs */
                    res->res = PQgetResult(res->handle);
                    if (res->res) {
                        res->ntuples = PQntuples(res->res);
                    }
                    else {
                        return -1;
                    }
                }
                if (res->sz == 0) {
                    res->sz = PQnfields(res->res);
                }
            }
            else {
                return -1;
            }
        }
    }
    return 0;
}
static const char *dbd_pgsql_get_entry(const apr_dbd_row *row, int n)
{
    return PQgetvalue(row->res->res, row->n, n);
}
static const char *dbd_pgsql_error(apr_dbd_t *sql, int n)
{
    return PQerrorMessage(sql);
}
static int dbd_pgsql_query(apr_dbd_t *sql, apr_dbd_transaction *trans,
                           int *nrows, const char *query)
{
    PGresult *res;
    int ret;
    if (trans && trans->errnum) {
        return trans->errnum;
    }
    res = PQexec(sql, query);
    if (res) {
        ret = PQresultStatus(res);
        if (dbd_pgsql_is_success(ret)) {
            /* ugh, making 0 return-success doesn't fit */
            ret = 0;
        }
        *nrows = atoi(PQcmdTuples(res));
        PQclear(res);
    }
    else {
        ret = PGRES_FATAL_ERROR;
    }
    if (trans) {
        trans->errnum = ret;
    }
    return ret;
}
static const char *dbd_pgsql_escape(apr_pool_t *pool, const char *arg,
                                    apr_dbd_t *sql)
{
    size_t len = strlen(arg);
    char *ret = apr_palloc(pool, len + 1);
    PQescapeString(ret, arg, len);
    return ret;
}
static int dbd_pgsql_prepare(apr_pool_t *pool, apr_dbd_t *sql,
                             const char *query, const char *label,
                             apr_dbd_prepared **statement)
{
    char *sqlcmd;
    char *sqlptr;
    size_t length;
    size_t i = 0;
    const char *args[QUERY_MAX_ARGS];
    size_t alen;
    int nargs = 0;
    int ret;
    PGresult *res;
    char *pgquery;
    char *pgptr;

    if (!*statement) {
        *statement = apr_palloc(pool, sizeof(apr_dbd_prepared));
    }
    /* Translate from apr_dbd to native query format */
    for (sqlptr = (char*)query; *sqlptr; ++sqlptr) {
        if ((sqlptr[0] == '%') && isalpha(sqlptr[1])) {
            ++nargs;
        }
    }
    length = strlen(query) + 1;
    if (nargs > 8) {
        length += nargs - 8;
    }
    pgptr = pgquery = apr_palloc(pool, length) ;

    for (sqlptr = (char*)query; *sqlptr; ++sqlptr) {
        if ((sqlptr[0] == '%') && isalpha(sqlptr[1])) {
            *pgptr++ = '$';
            if (i < 9) {
                *pgptr++ = '1' + i;
            }
            else {
                *pgptr++ = '0' + ((i+1)/10);
                *pgptr++ = '0' + ((i+1)%10);
            }
            switch (*++sqlptr) {
            case 'd':
                args[i] = "integer";
                break;
            case 's':
                args[i] = "varchar";
                break;
            default:
                args[i] = "varchar";
                break;
            }
            length += 1 + strlen(args[i]);
            ++i;
        }
        else if ((sqlptr[0] == '%') && (sqlptr[1] == '%')) {
            /* reduce %% to % */
            *pgptr++ = *sqlptr++;
        }
        else {
            *pgptr++ = *sqlptr;
        }
    }
    *pgptr = 0;

    if (!label) {
        /* don't really prepare; use in execParams instead */
        (*statement)->prepared = 0;
        (*statement)->name = apr_pstrdup(pool, pgquery);
        return 0;
    }
    (*statement)->name = apr_pstrdup(pool, label);

    /* length of SQL query that prepares this statement */
    length = 8 + strlen(label) + 2 + 4 + length + 1;
    sqlcmd = apr_palloc(pool, length);
    sqlptr = sqlcmd;
    memcpy(sqlptr, "PREPARE ", 8);
    sqlptr += 8;
    length = strlen(label);
    memcpy(sqlptr, label, length);
    sqlptr += length;
    if (nargs > 0) {
        memcpy(sqlptr, " (",2);
        sqlptr += 2;
        for (i=0; i<nargs; ++i) {
            alen = strlen(args[i]);
            memcpy(sqlptr, args[i], alen);
            sqlptr += alen;
            *sqlptr++ = ',';
        }
        sqlptr[-1] =  ')';
    }
    memcpy(sqlptr, " AS ", 4);
    sqlptr += 4;
    memcpy(sqlptr, pgquery, strlen(pgquery));
    sqlptr += strlen(pgquery);
    *sqlptr = 0;

    res = PQexec(sql, sqlcmd);
    if ( res ) {
        ret = PQresultStatus(res);
        if (dbd_pgsql_is_success(ret)) {
            ret = 0;
        }
        /* Hmmm, do we do this here or register it on the pool? */
        PQclear(res);
    }
    else {
        ret = PGRES_FATAL_ERROR;
    }
    (*statement)->prepared = 1;

    return ret;
}
static int dbd_pgsql_pquery(apr_pool_t *pool, apr_dbd_t *sql,
                            apr_dbd_transaction *trans, int *nrows,
                            apr_dbd_prepared *statement, int nargs,
                            const char **values)
{
    int ret;
    PGresult *res;
    if (statement->prepared) {
        res = PQexecPrepared(sql, statement->name, nargs, values, 0, 0, 0);
    }
    else {
        res = PQexecParams(sql, statement->name, nargs, 0, values, 0, 0, 0);
    }
    if (res) {
        ret = PQresultStatus(res);
        if (dbd_pgsql_is_success(ret)) {
            ret = 0;
        }
        PQclear(res);
    }
    else {
        ret = PGRES_FATAL_ERROR;
    }

    if (trans) {
        trans->errnum = ret;
    }
    return ret;
}
static int dbd_pgsql_pvquery(apr_pool_t *pool, apr_dbd_t *sql,
                             apr_dbd_transaction *trans, int *nrows,
                             apr_dbd_prepared *statement, ...)
{
    const char *arg;
    int nargs = 0;
    va_list args;
    const char *values[QUERY_MAX_ARGS];

    if (trans && trans->errnum) {
        return trans->errnum;
    }
    va_start(args, statement);
    while ( arg = va_arg(args, const char*), arg ) {
        if ( nargs >= QUERY_MAX_ARGS) {
            va_end(args);
            return -1;
        }
        values[nargs++] = apr_pstrdup(pool, arg);
    }
    va_end(args);
    values[nargs] = NULL;
    return dbd_pgsql_pquery(pool, sql, trans, nrows, statement, nargs, values);
}
static int dbd_pgsql_pselect(apr_pool_t *pool, apr_dbd_t *sql,
                             apr_dbd_transaction *trans,
                             apr_dbd_results **results,
                             apr_dbd_prepared *statement,
                             int seek, int nargs, const char **values)
{
    PGresult *res;
    int rv;
    int ret = 0;
    if (seek) { /* synchronous query */
        if (statement->prepared) {
            res = PQexecPrepared(sql, statement->name, nargs, values, 0, 0, 0);
        }
        else {
            res = PQexecParams(sql, statement->name, nargs, 0, values, 0, 0, 0);
        }
        if (res) {
            ret = PQresultStatus(res);
            if (dbd_pgsql_is_success(ret)) {
                ret = 0;
            }
            else {
                PQclear(res);
            }
        }
        else {
            ret = PGRES_FATAL_ERROR;
        }
        if (ret != 0) {
            if (trans) {
                trans->errnum = ret;
            }
            return ret;
        }
        if (!*results) {
            *results = apr_pcalloc(pool, sizeof(apr_dbd_results));
        }
        (*results)->res = res;
        (*results)->ntuples = PQntuples(res);
        (*results)->sz = PQnfields(res);
        (*results)->random = seek;
        apr_pool_cleanup_register(pool, res, (void*)PQclear,
                                  apr_pool_cleanup_null);
    }
    else {
        if (statement->prepared) {
            rv = PQsendQueryPrepared(sql, statement->name, nargs, values,0,0,0);
        }
        else {
            rv = PQsendQueryParams(sql, statement->name, nargs, 0, values,0,0,0);
        }
        if (rv == 0) {
            if (trans) {
                trans->errnum = 1;
            }
            return 1;
        }
        if (!*results) {
            *results = apr_pcalloc(pool, sizeof(apr_dbd_results));
        }
        (*results)->random = seek;
        (*results)->handle = sql;
    }

    if (trans) {
        trans->errnum = ret;
    }
    return ret;
}
static int dbd_pgsql_pvselect(apr_pool_t *pool, apr_dbd_t *sql,
                              apr_dbd_transaction *trans,
                              apr_dbd_results **results,
                              apr_dbd_prepared *statement,
                              int seek, ...)
{
    const char *arg;
    int nargs = 0;
    va_list args;
    const char *values[QUERY_MAX_ARGS];

    if (trans && trans->errnum) {
        return trans->errnum;
    }

    va_start(args, seek);
    while (arg = va_arg(args, const char*), arg) {
        if ( nargs >= QUERY_MAX_ARGS) {
            va_end(args);
            return -1;
        }
        values[nargs++] = apr_pstrdup(pool, arg);
    }
    va_end(args);
    return dbd_pgsql_pselect(pool, sql, trans, results, statement,
                             seek, nargs, values) ;
}
static int dbd_pgsql_transaction(apr_pool_t *pool, apr_dbd_t *handle,
                                 apr_dbd_transaction **trans)
{
    int ret = 0;
    PGresult *res = PQexec(handle, "BEGIN TRANSACTION");
    if (res) {
        ret = PQresultStatus(res);
        if (dbd_pgsql_is_success(ret)) {
            ret = 0;
            if (!*trans) {
                *trans = apr_pcalloc(pool, sizeof(apr_dbd_transaction));
            }
        }
        PQclear(res);
        (*trans)->handle = handle;
    }
    else {
        ret = PGRES_FATAL_ERROR;
    }
    return ret;
}
static int dbd_pgsql_end_transaction(apr_dbd_transaction *trans)
{
    PGresult *res;
    int ret = -1;                /* no transaction is an error cond */
    if (trans) {
        if (trans->errnum) {
            trans->errnum = 0;
            res = PQexec(trans->handle, "ROLLBACK");
        }
        else {
            res = PQexec(trans->handle, "COMMIT");
        }
        if (res) {
            ret = PQresultStatus(res);
            if (dbd_pgsql_is_success(ret)) {
                ret = 0;
            }
            PQclear(res);
        }
        else {
            ret = PGRES_FATAL_ERROR;
        }
    }
    return ret;
}
static apr_dbd_t *dbd_pgsql_open(apr_pool_t *pool, const char *params)
{
    return PQconnectdb(params);
}
static apr_status_t dbd_pgsql_close(apr_dbd_t *handle)
{
    PQfinish(handle);
    return APR_SUCCESS;
}
static apr_status_t dbd_pgsql_check_conn(apr_pool_t *pool,
                                         apr_dbd_t *handle)
{
    if (PQstatus(handle) != CONNECTION_OK) {
        PQreset(handle);
        if (PQstatus(handle) != CONNECTION_OK) {
            return APR_EGENERAL;
        }
    }
    return APR_SUCCESS;
}
static int dbd_pgsql_select_db(apr_pool_t *pool, apr_dbd_t *handle,
                               const char *name)
{
    return APR_ENOTIMPL;
}
static void *dbd_pgsql_native(apr_dbd_t *handle)
{
    return handle;
}
static int dbd_pgsql_num_cols(apr_dbd_results* res)
{
    return res->sz;
}
static int dbd_pgsql_num_tuples(apr_dbd_results* res)
{
    if (res->random) {
        return res->ntuples;
    }
    else {
        return -1;
    }
}
APU_DECLARE_DATA const apr_dbd_driver_t apr_dbd_pgsql_driver = {
    "pgsql",
    NULL,
    dbd_pgsql_native,
    dbd_pgsql_open,
    dbd_pgsql_check_conn,
    dbd_pgsql_close,
    dbd_pgsql_select_db,
    dbd_pgsql_transaction,
    dbd_pgsql_end_transaction,
    dbd_pgsql_query,
    dbd_pgsql_select,
    dbd_pgsql_num_cols,
    dbd_pgsql_num_tuples,
    dbd_pgsql_get_row,
    dbd_pgsql_get_entry,
    dbd_pgsql_error,
    dbd_pgsql_escape,
    dbd_pgsql_prepare,
    dbd_pgsql_pvquery,
    dbd_pgsql_pvselect,
    dbd_pgsql_pquery,
    dbd_pgsql_pselect,
};
#endif
