/* Copyright 2000-2004 The Apache Software Foundation
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

/*
 * apr_ldap_compat.c: LDAP v2/v3 compatibility things
 * 
 * Original code from auth_ldap module for Apache v1.3:
 * Copyright 1998, 1999 Enbridge Pipelines Inc. 
 * Copyright 1999-2001 Dave Carrigan
 */

#include <apr_ldap.h>

#if APR_HAS_LDAP


/* 
 * Compatibility for LDAPv2 libraries. Differences between LDAPv2 and 
 * LDAPv3, as they affect this module
 * 
 *  LDAPv3 uses ldap_search_ext_s; LDAPv2 uses only basic ldap_search_s
 *
 *  LDAPv3 uses ldap_memfree; LDAPv2 just uses free().
 *
 * In this section, we just implement the LDAPv3 SDK functions that are 
 * missing in LDAPv2. 
 * 
 */

/* Note: This section has been removed in APR v1.0, pending real
 * support for LDAP v2.0 toolkits.
 *
 * In the mean time, please use an LDAP v3.0 toolkit.
 */

#endif /* APR_HAS_LDAP */
