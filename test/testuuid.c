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

#include "test_aprutil.h"
#include "apr_general.h"
#include "apr_uuid.h"

static void test_uuid_parse(CuTest *tc)
{
    apr_uuid_t uuid;
    apr_uuid_t uuid2;
    char buf[APR_UUID_FORMATTED_LENGTH + 1];

    apr_uuid_get(&uuid);
    apr_uuid_format(buf, &uuid);

    apr_uuid_parse(&uuid2, buf);
    CuAssert(tc, "parse produced a different UUID",
             memcmp(&uuid, &uuid2, sizeof(uuid)) == 0);
}

static void test_gen2(CuTest *tc)
{
    apr_uuid_t uuid;
    apr_uuid_t uuid2;

    /* generate two of them quickly */
    apr_uuid_get(&uuid);
    apr_uuid_get(&uuid2);

    CuAssert(tc, "generated the same UUID twice",
             memcmp(&uuid, &uuid2, sizeof(uuid)) != 0);
}

CuSuite *testuuid(void)
{
    CuSuite *suite = CuSuiteNew("UUID");

    SUITE_ADD_TEST(suite, test_uuid_parse);
    SUITE_ADD_TEST(suite, test_gen2);

    return suite;
}
