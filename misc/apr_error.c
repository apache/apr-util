/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * levelations under the License.
 */

#include "apu.h"
#include "apr_strings.h"
#include "apr_pools.h"
#include "apu_errno.h"

APU_DECLARE_NONSTD(apu_err_t *) apr_errprintf(apu_err_t *result,
        apr_pool_t *p, const char *reason, int rc, const char *fmt, ...)
{
    va_list ap;

    if (!result) {
        result = apr_pcalloc(p, sizeof(apu_err_t));
        if (!result) {
            return NULL;
        }
    }

    va_start(ap, fmt);
    result->msg = apr_pvsprintf(p, fmt, ap);
    va_end(ap);

    result->reason = reason;
    result->rc = rc;

    return result;
}

/*
 * stuffbuffer - like apr_cpystrn() but returns the address of the
 * dest buffer instead of the address of the terminating '\0'
 */
static char *stuffbuffer(char *buf, apr_size_t bufsize, const char *s)
{
    apr_cpystrn(buf,s,bufsize);
    return buf;
}

static char *apu_error_string(apr_status_t statcode)
{
    switch (statcode) {
    case APR_ENOKEY:         
        return "The key provided was empty or NULL";
    case APR_ENOIV:
        return "The initialisation vector provided was NULL";
    case APR_EKEYTYPE:
        return "The key type was not recognised";
    case APR_ENOSPACE:
        return "The buffer supplied was not big enough";
    case APR_ECRYPT:
        return "Internal error in the crypto subsystem (specific information not available)";
    case APR_EPADDING:
        return "Padding was not supported";
    case APR_EKEYLENGTH:
        return "The key length was incorrect";
    case APR_ENOCIPHER:
        return "The cipher provided was not recognised";
    case APR_ENODIGEST:
        return "The digest provided was not recognised";
    case APR_ENOENGINE:
        return "No engine found for crypto subsystem";
    case APR_EINITENGINE:
        return "Failed to init engine for crypto subsystem";
    case APR_EREINIT:        
        return "Underlying crypto has already been initialised";
    case APR_ENOVERIFY:
        return "The signature verification failed";

    case APR_WANT_READ:
        return "Call me again when the socket is ready for reading";
    case APR_WANT_WRITE:
        return "Call me again when the socket is ready for writing";

    case APR_OTHER:
    	return "An internal LDAP error has occurred";
    case APR_SERVER_DOWN:
        return "The server is down";
    case APR_LOCAL_ERROR:
        return "An error has occurred locally";
    case APR_ENCODING_ERROR:
        return "Encoding has failed";
    case APR_DECODING_ERROR:
        return "Decoding has failed";
    case APR_AUTH_UNKNOWN:
        return "Unknown SASL mechanism";
    case APR_FILTER_ERROR:
        return "The filter was malformed";
    case APR_USER_CANCELLED:
        return "User has cancelled the request";
    case APR_PARAM_ERROR:
    	return "Parameter error";
    case APR_CONNECT_ERROR:
    	return "Connect error";
    case APR_NOT_SUPPORTED:
    	return "Not supported";
    case APR_CONTROL_NOT_FOUND:
    	return "Control not found";
    case APR_NO_RESULTS_RETURNED:
    	return "No results returned";
    case APR_MORE_RESULTS_TO_RETURN:
    	return "More results to be returned";
    case APR_CLIENT_LOOP:
    	return "Client loop has occurred";
    case APR_REFERRAL_LIMIT_EXCEEDED:
    	return "Referral limit exceeded";
    case APR_CONNECTING:
    	return "Connecting";

    case APR_OPERATIONS_ERROR:
    	return "Operations error has occurred";
    case APR_PROTOCOL_ERROR:
    	return "Protocol error has occurred";
    case APR_TIMELIMIT_EXCEEDED:
    	return "Time limit has been exceeded";
    case APR_SIZELIMIT_EXCEEDED:
    	return "Size limit has been exceeded";
    case APR_PROXY_AUTH:
        return "Proxy authorization has failed";
    case APR_INAPPROPRIATE_AUTH:
        return "Authentication not appropriate for this entry";
    case APR_INVALID_CREDENTIALS:
        return "Invalid credentials were presented";
    case APR_INSUFFICIENT_ACCESS:
        return "The user has insufficient access";
    case APR_UNAVAILABLE:
        return "Unavailable";
    case APR_CONSTRAINT_VIOLATION:
        return "A constraint was violated";
    case APR_NO_SUCH_OBJECT:
        return "No such object";
    case APR_NO_SUCH_ATTRIBUTE:
        return "No such attribute";
    case APR_COMPARE_TRUE:
        return "Comparison is true";
    case APR_COMPARE_FALSE:
        return "Comparison is false";
    case APR_ALREADY_EXISTS:
        return "The object already exists";
    case APR_OBJECT_CLASS_VIOLATION:
        return "Add or modify results in an objectclass violation";

    default:
        return "Error string not specified yet";
    }
}

APU_DECLARE(char *) apu_strerror(apr_status_t statcode, char *buf,
                                 apr_size_t bufsize)
{
    if (statcode < APR_UTIL_START_STATUS) {
        return apr_strerror(statcode, buf, bufsize);
    }
    else if (statcode < (APR_UTIL_START_STATUS + APR_UTIL_ERRSPACE_SIZE)) {
        return stuffbuffer(buf, bufsize, apu_error_string(statcode));
    }
    else {
        return apr_strerror(statcode, buf, bufsize);
    }
}

