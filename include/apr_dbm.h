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

#ifndef APU_DBM_H
#define APU_DBM_H

#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct apu_dbm_t apu_dbm_t;

typedef struct
{
    char *dptr;
    apr_size_t dsize;
} apu_datum_t;

/* modes to open the DB */
#define APU_DBM_READONLY        1       /* open for read-only access */
#define APU_DBM_READWRITE       2       /* open for read-write access */
#define APU_DBM_RWCREATE        3       /* open for r/w, create if needed */


apr_status_t apu_dbm_open(const char *pathname, apr_pool_t *pool, int mode,
                          apu_dbm_t **pdb);
void apu_dbm_close(apu_dbm_t *db);
apr_status_t apu_dbm_fetch(apu_dbm_t *db, apu_datum_t key,
                           apu_datum_t *pvalue);
apr_status_t apu_dbm_store(apu_dbm_t *db, apu_datum_t key, apu_datum_t value);
apr_status_t apu_dbm_delete(apu_dbm_t *db, apu_datum_t key);
int apu_dbm_exists(apu_dbm_t *db, apu_datum_t key);
apr_status_t apu_dbm_firstkey(apu_dbm_t *db, apu_datum_t *pkey);
apr_status_t apu_dbm_nextkey(apu_dbm_t *db, apu_datum_t *pkey);
void apu_dbm_freedatum(apu_dbm_t *db, apu_datum_t data);

void apu_dbm_geterror(apu_dbm_t *db, int *errcode, const char **errmsg);

#ifdef __cplusplus
}
#endif

#endif	/* !APU_DBM_H */


