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
#define APR_DBM_DBMODE_RWTRUNC  (APR_READ | APR_WRITE | APR_CREATE|APR_TRUNCATE)
