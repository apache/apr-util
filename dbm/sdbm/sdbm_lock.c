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

#include "apr_file_info.h"
#include "apr_file_io.h"
#include "apr_sdbm.h"

#include "sdbm_private.h"
#include "sdbm_tune.h"

/* NOTE: this function blocks until it acquires the lock */
APU_DECLARE(apr_status_t) apr_sdbm_lock(apr_sdbm_t *db, int type)
{
    apr_status_t status;

    if (!(type == APR_FLOCK_SHARED || type == APR_FLOCK_EXCLUSIVE))
        return APR_EINVAL;

    if (db->flags & SDBM_EXCLUSIVE_LOCK) {
        ++db->lckcnt;
        return APR_SUCCESS;
    }
    else if (db->flags & SDBM_SHARED_LOCK) {
        /*
         * Cannot promote a shared lock to an exlusive lock
         * in a cross-platform compatibile manner.
         */
        if (type == APR_FLOCK_EXCLUSIVE)
            return APR_EINVAL;
        ++db->lckcnt;
        return APR_SUCCESS;
    }
    /*
     * zero size: either a fresh database, or one with a single,
     * unsplit data page: dirpage is all zeros.
     */
    if ((status = apr_file_lock(db->dirf, type)) == APR_SUCCESS) 
    {
        apr_finfo_t finfo;
        if ((status = apr_file_info_get(&finfo, APR_FINFO_SIZE, db->dirf))
                != APR_SUCCESS) {
            (void) apr_file_unlock(db->dirf);
            return status;
        }

        SDBM_INVALIDATE_CACHE(db, finfo);

        ++db->lckcnt;
        if (type == APR_FLOCK_SHARED)
            db->flags |= SDBM_SHARED_LOCK;
        else if (type == APR_FLOCK_EXCLUSIVE)
            db->flags |= SDBM_EXCLUSIVE_LOCK;
    }
    return status;
}

APU_DECLARE(apr_status_t) apr_sdbm_unlock(apr_sdbm_t *db)
{
    if (!(db->flags & (SDBM_SHARED_LOCK | SDBM_EXCLUSIVE_LOCK)))
        return APR_EINVAL;
    if (--db->lckcnt > 0)
        return APR_SUCCESS;
    db->flags &= ~(SDBM_SHARED_LOCK | SDBM_EXCLUSIVE_LOCK);
    return apr_file_unlock(db->dirf);
}
