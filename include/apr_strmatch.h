/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002-2003 The Apache Software Foundation.  All rights
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
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

#ifndef APR_STRMATCH_H
#define APR_STRMATCH_H
/**
 * @file apr_strmatch.h
 * @brief APR-UTIL string matching routines
 */

#include "apu.h"
#include "apr_pools.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup APR_Util_StrMatch String matching routines
 * @ingroup APR_Util
 * @{
 */

/** @see apr_strmatch_pattern */
typedef struct apr_strmatch_pattern apr_strmatch_pattern;

/**
 * Precompiled search pattern
 */
struct apr_strmatch_pattern {
    /** Function called to compare */
    const char *(*compare)(const apr_strmatch_pattern *this_pattern,
                           const char *s, apr_size_t slen);
    const char *pattern;    /**< Current pattern */
    apr_size_t length;      /**< Current length */
    void *context;          /**< hook to add precomputed metadata */
};

#if defined(DOXYGEN)
/**
 * Search for a precompiled pattern within a string
 * @param pattern The pattern
 * @param s The string in which to search for the pattern
 * @param slen The length of s (excluding null terminator)
 * @return A pointer to the first instance of the pattern in s, or
 *         NULL if not found
 */
APU_DECLARE(const char *) apr_strmatch(const apr_strmatch_pattern *pattern,
                                       const char *s, apr_size_t slen);
#else
#define apr_strmatch(pattern, s, slen) (*((pattern)->compare))((pattern), (s), (slen))
#endif

/**
 * Precompile a pattern for matching using the Boyer-Moore-Horspool algorithm
 * @param p The pool from which to allocate the pattern
 * @param s The pattern string
 * @param case_sensitive Whether the matching should be case-sensitive
 * @return a pointer to the compiled pattern, or NULL if compilation fails
 */
APU_DECLARE(const apr_strmatch_pattern *) apr_strmatch_precompile(apr_pool_t *p, const char *s, int case_sensitive);

/** @} */
#ifdef __cplusplus
}
#endif

#endif	/* !APR_STRMATCH_H */
