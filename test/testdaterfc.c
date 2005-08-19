/* Copyright 2000-2005 The Apache Software Foundation or its licensors, as
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

#include "testutil.h"
#include "apr_date.h"

struct datetest {
  const char *input;
  const char *output;
} tests[] = {
  { "Mon, 27 Feb 1995 20:49:44 -0800",  "Tue, 28 Feb 1995 04:49:44 GMT" },
  { "Fri,  1 Jul 2005 11:34:25 -0400",  "Fri, 01 Jul 2005 15:34:25 GMT" },
  { "Monday, 27-Feb-95 20:49:44 -0800", "Tue, 28 Feb 1995 04:49:44 GMT" },
  { "Tue, 4 Mar 1997 12:43:52 +0200",   "Tue, 04 Mar 1997 10:43:52 GMT" },
  { "Mon, 27 Feb 95 20:49:44 -0800",    "Tue, 28 Feb 1995 04:49:44 GMT" },
  { "Tue,  4 Mar 97 12:43:52 +0200",    "Tue, 04 Mar 1997 10:43:52 GMT" },
  { "Tue, 4 Mar 97 12:43:52 +0200",     "Tue, 04 Mar 1997 10:43:52 GMT" },
  { "Mon, 27 Feb 95 20:49 GMT",         "Mon, 27 Feb 1995 20:49:00 GMT" },
  { "Tue, 4 Mar 97 12:43 GMT",          "Tue, 04 Mar 1997 12:43:00 GMT" },
  { NULL, NULL }
};

static void test_date_rfc(abts_case *tc, void *data)
{
    apr_time_t date;
    int i = 0;

    while (tests[i].input) {
        char str_date[APR_RFC822_DATE_LEN] = { 0 };

        date = apr_date_parse_rfc(tests[i].input);

        apr_rfc822_date(str_date, date);

        ABTS_STR_EQUAL(tc, str_date, tests[i].output);

        i++;
    }
}

abts_suite *testdaterfc(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_date_rfc, NULL);

    return suite;
}
