/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
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
 * This is derived from material copyright RSA Data Security, Inc.
 * Their notice is reproduced below in its entirety.
 *
 * Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD4 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 *
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD4 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

#ifndef APR_MD4_H
#define APR_MD4_H

#include "apu.h"
#include "apr_xlate.h"
/**
 * @file apr_md4.h
 * @brief APR-UTIL MD4 Library
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup APR_Util_MD4 MD4 Library
 * @ingroup APR_Util
 * @{
 */

/** The digestsize for MD4 */
#define APR_MD4_DIGESTSIZE 16

/** @see apr_md4_ctx_t */
typedef struct apr_md4_ctx_t apr_md4_ctx_t;

/** MD4 context. */
struct apr_md4_ctx_t {
    /** state (ABCD) */
    apr_uint32_t state[4];
    /** number of bits, modulo 2^64 (lsb first) */
    apr_uint32_t count[2];
    /** input buffer */
    unsigned char buffer[64];
#if APR_HAS_XLATE
    /** translation handle */
    apr_xlate_t *xlate;
#endif
};

/**
 * MD4 Initialize.  Begins an MD4 operation, writing a new context.
 * @param context The MD4 context to initialize.
 */
APU_DECLARE(apr_status_t) apr_md4_init(apr_md4_ctx_t *context);

#if APR_HAS_XLATE
/**
 * MDr4 translation setup.  Provides the APR translation handle to be used 
 * for translating the content before calculating the digest.
 * @param context The MD4 content to set the translation for.
 * @param xlate The translation handle to use for this MD4 context 
 */
APU_DECLARE(apr_status_t) apr_md4_set_xlate(apr_md4_ctx_t *context,
                                            apr_xlate_t *xlate);
#else
#define apr_md4_set_xlate(context, xlate) APR_ENOTIMPL
#endif

/**
 * MD4 block update operation.  Continue an MD4 message-digest operation, 
 * processing another message block, and updating the context.
 * @param context The MD4 content to update.
 * @param input next message block to update
 * @param inputLen The length of the next message block
 */
APU_DECLARE(apr_status_t) apr_md4_update(apr_md4_ctx_t *context,
                                         const unsigned char *input,
                                         apr_size_t inputLen);

/**
 * MD4 finalization.  Ends an MD4 message-digest operation, writing the 
 * message digest and zeroing the context
 * @param digest The final MD4 digest
 * @param context The MD4 content we are finalizing.
 */
APU_DECLARE(apr_status_t) apr_md4_final(
                                    unsigned char digest[APR_MD4_DIGESTSIZE],
                                    apr_md4_ctx_t *context);

/**
 * MD4 digest computation
 * @param digest The MD4 digest
 * @param input message block to use
 * @param inputLen The length of the message block
 */
APU_DECLARE(apr_status_t) apr_md4(unsigned char digest[APR_MD4_DIGESTSIZE],
                                  const unsigned char *input,
                                  apr_size_t inputLen);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* !APR_MD4_H */
