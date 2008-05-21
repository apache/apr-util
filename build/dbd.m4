dnl -------------------------------------------------------- -*- autoconf -*-
dnl Licensed to the Apache Software Foundation (ASF) under one or more
dnl contributor license agreements.  See the NOTICE file distributed with
dnl this work for additional information regarding copyright ownership.
dnl The ASF licenses this file to You under the Apache License, Version 2.0
dnl (the "License"); you may not use this file except in compliance with
dnl the License.  You may obtain a copy of the License at
dnl
dnl     http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl Unless required by applicable law or agreed to in writing, software
dnl distributed under the License is distributed on an "AS IS" BASIS,
dnl WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl See the License for the specific language governing permissions and
dnl limitations under the License.

dnl
dnl DBD module
dnl

dnl
dnl APU_CHECK_DBD: compile backends for apr_dbd.
dnl
AC_DEFUN([APU_CHECK_DBD], [
  apu_have_pgsql=0

  old_libs="$LIBS"
  old_cppflags="$CPPFLAGS"
  old_ldflags="$LDFLAGS"

  AC_ARG_WITH([pgsql],
    APR_HELP_STRING([--with-pgsql=DIR], [specify PostgreSQL location]),
  [
    if test "$withval" = "yes"; then
      AC_PATH_PROG([PGSQL_CONFIG],[pg_config])
      if test "x$PGSQL_CONFIG" != 'x'; then
        pgsql_CPPFLAGS="-I`$PGSQL_CONFIG --includedir`"
        pgsql_LDFLAGS="-L`$PGSQL_CONFIG --libdir`"
        pgsql_LIBS="`$PGSQL_CONFIG --libs`"

        APR_ADDTO(CPPFLAGS, [$pgsql_CPPFLAGS])
        APR_ADDTO(LDFLAGS, [$pgsql_LDFLAGS])
        APR_ADDTO(LIBS, [$pgsql_LIBS])
      fi

      AC_CHECK_HEADERS(libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
      if test "$apu_have_pgsql" = "0"; then
        AC_CHECK_HEADERS(postgresql/libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
      fi
      if test "$apu_have_pgsql" != "0" && test "x$PGSQL_CONFIG" != 'x'; then
        APR_ADDTO(APRUTIL_INCLUDES, [$pgsql_CPPFLAGS])
      fi
    elif test "$withval" = "no"; then
      :
    else
      AC_PATH_PROG([PGSQL_CONFIG],[pg_config],,[$withval/bin])
      if test "x$PGSQL_CONFIG" != 'x'; then
        pgsql_CPPFLAGS="-I`$PGSQL_CONFIG --includedir`"
        pgsql_LDFLAGS="-L`$PGSQL_CONFIG --libdir`"
        pgsql_LIBS="`$PGSQL_CONFIG --libs`"
      else
        pgsql_CPPFLAGS="-I$withval/include"
        pgsql_LDFLAGS="-L$withval/lib "
      fi

      APR_ADDTO(CPPFLAGS, [$pgsql_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$pgsql_LDFLAGS])
      APR_ADDTO(LIBS, [$pgsql_LIBS])

      AC_MSG_NOTICE(checking for pgsql in $withval)
      AC_CHECK_HEADERS(libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
      if test "$apu_have_pgsql" != "1"; then
        AC_CHECK_HEADERS(postgresql/libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
      fi
      if test "$apu_have_pgsql" != "0"; then
        APR_ADDTO(APRUTIL_INCLUDES, [$pgsql_CPPFLAGS])
      fi
    fi
  ], [
    AC_PATH_PROG([PGSQL_CONFIG],[pg_config])
    if test "x$PGSQL_CONFIG" != 'x'; then
      pgsql_CPPFLAGS="-I`$PGSQL_CONFIG --includedir`"
      pgsql_LDFLAGS="-L`$PGSQL_CONFIG --libdir`"
      pgsql_LIBS="`$PGSQL_CONFIG --libs`"

      APR_ADDTO(CPPFLAGS, [$pgsql_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$pgsql_LDFLAGS])
      APR_ADDTO(LIBS, [$pgsql_LIBS])
    fi

    AC_CHECK_HEADERS(libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
    if test "$apu_have_pgsql" = "0"; then
      AC_CHECK_HEADERS(postgresql/libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
    fi
    if test "$apu_have_pgsql" != "0" && test "x$PGSQL_CONFIG" != 'x'; then
      APR_ADDTO(APRUTIL_INCLUDES, [$pgsql_CPPFLAGS])
    fi
  ])
  AC_SUBST(apu_have_pgsql)
  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_pgsql" = "1"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[$pgsql_LDFLAGS -lpq $pgsql_LIBS])
    APR_ADDTO(APRUTIL_LIBS,[$pgsql_LDFLAGS -lpq $pgsql_LIBS])
  fi

  LIBS="$old_libs"
  CPPFLAGS="$old_cppflags"
  LDFLAGS="$old_ldflags"
])
dnl
AC_DEFUN([APU_CHECK_DBD_MYSQL], [
  apu_have_mysql=0

  old_libs="$LIBS"
  old_cppflags="$CPPFLAGS"
  old_ldflags="$LDFLAGS"

  AC_ARG_WITH([mysql],
    APR_HELP_STRING([--with-mysql=DIR], [specify MySQL location (disabled by default)]),
  [
    if test "$withval" = "yes"; then
      AC_PATH_PROG([MYSQL_CONFIG],[mysql_config])
      if test "x$MYSQL_CONFIG" != 'x'; then
        mysql_CPPFLAGS="`$MYSQL_CONFIG --include`"
        mysql_LDFLAGS="`$MYSQL_CONFIG --libs_r | sed -e 's/-l[[^ ]]\+//g'`"
        mysql_LIBS="`$MYSQL_CONFIG --libs_r`"

        APR_ADDTO(CPPFLAGS, [$mysql_CPPFLAGS])
        APR_ADDTO(LIBS, [$mysql_LIBS])
      fi

      AC_CHECK_HEADERS(mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
      if test "$apu_have_mysql" = "0"; then
        AC_CHECK_HEADERS(mysql/mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
      fi
      if test "$apu_have_mysql" != "0" && test "x$MYSQL_CONFIG" != 'x'; then
        APR_ADDTO(APRUTIL_INCLUDES, [$mysql_CPPFLAGS])
      fi
    elif test "$withval" = "no"; then
      :
    else
      AC_PATH_PROG([MYSQL_CONFIG],[mysql_config],,[$withval/bin])
      if test "x$MYSQL_CONFIG" != 'x'; then
        mysql_CPPFLAGS="`$MYSQL_CONFIG --include`"
        mysql_LDFLAGS="`$MYSQL_CONFIG --libs_r | sed -e 's/-l[[^ ]]\+//g'`"
        mysql_LIBS="`$MYSQL_CONFIG --libs_r`"
      else
        mysql_CPPFLAGS="-I$withval/include"
        mysql_LDFLAGS="-L$withval/lib "
      fi

      APR_ADDTO(CPPFLAGS, [$mysql_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$mysql_LDFLAGS])
      APR_ADDTO(LIBS, [$mysql_LIBS])

      AC_MSG_NOTICE(checking for mysql in $withval)
      AC_CHECK_HEADERS(mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))

      if test "$apu_have_mysql" != "1"; then
        AC_CHECK_HEADERS(mysql/mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
      fi
      if test "$apu_have_mysql" != "0"; then
        APR_ADDTO(APRUTIL_INCLUDES, [$mysql_CPPFLAGS])
      fi
    fi
  ])

  AC_SUBST(apu_have_mysql)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_mysql" = "1"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[$mysql_LDFLAGS -lmysqlclient_r $mysql_LIBS])
    APR_ADDTO(APRUTIL_LIBS,[$mysql_LDFLAGS -lmysqlclient_r $mysql_LIBS])
  fi

  LIBS="$old_libs"
  CPPFLAGS="$old_cppflags"
  LDFLAGS="$old_ldflags"
])
dnl
AC_DEFUN([APU_CHECK_DBD_SQLITE3], [
  apu_have_sqlite3=0

  old_libs="$LIBS"
  old_cppflags="$CPPFLAGS"
  old_ldflags="$LDFLAGS"

  AC_ARG_WITH([sqlite3],
    APR_HELP_STRING([--with-sqlite3=DIR], [enable sqlite3 DBD driver]),
  [
    if test "$withval" = "yes"; then
      AC_CHECK_HEADERS(sqlite3.h, AC_CHECK_LIB(sqlite3, sqlite3_open, [apu_have_sqlite3=1]))
    elif test "$withval" = "no"; then
      :
    else
      sqlite3_CPPFLAGS="-I$withval/include"
      sqlite3_LDFLAGS="-L$withval/lib "

      APR_ADDTO(CPPFLAGS, [$sqlite3_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$sqlite3_LDFLAGS])

      AC_MSG_NOTICE(checking for sqlite3 in $withval)
      AC_CHECK_HEADERS(sqlite3.h, AC_CHECK_LIB(sqlite3, sqlite3_open, [apu_have_sqlite3=1]))
      if test "$apu_have_sqlite3" != "0"; then
        APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
      fi
    fi
  ], [
    AC_CHECK_HEADERS(sqlite3.h, AC_CHECK_LIB(sqlite3, sqlite3_open, [apu_have_sqlite3=1]))
  ])

  AC_SUBST(apu_have_sqlite3)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_sqlite3" = "1"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[$sqlite3_LDFLAGS -lsqlite3])
    APR_ADDTO(APRUTIL_LIBS,[$sqlite3_LDFLAGS -lsqlite3])
  fi

  LIBS="$old_libs"
  CPPFLAGS="$old_cppflags"
  LDFLAGS="$old_ldflags"
])
dnl
AC_DEFUN([APU_CHECK_DBD_SQLITE2], [
  apu_have_sqlite2=0

  old_libs="$LIBS"
  old_cppflags="$CPPFLAGS"
  old_ldflags="$LDFLAGS"

  AC_ARG_WITH([sqlite2],
    APR_HELP_STRING([--with-sqlite2=DIR], [enable sqlite2 DBD driver]),        
  [
    if test "$withval" = "yes"; then
      AC_CHECK_HEADERS(sqlite.h, AC_CHECK_LIB(sqlite, sqlite_open, [apu_have_sqlite2=1]))
    elif test "$withval" = "no"; then
      :
    else
      sqlite2_CPPFLAGS="-I$withval/include"
      sqlite2_LDFLAGS="-L$withval/lib "

      APR_ADDTO(CPPFLAGS, [$sqlite2_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$sqlite2_LDFLAGS])

      AC_MSG_NOTICE(checking for sqlite2 in $withval)
      AC_CHECK_HEADERS(sqlite.h, AC_CHECK_LIB(sqlite, sqlite_open, [apu_have_sqlite2=1]))
      if test "$apu_have_sqlite2" != "0"; then
        APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
      fi
    fi
  ], [
    AC_CHECK_HEADERS(sqlite.h, AC_CHECK_LIB(sqlite, sqlite_open, [apu_have_sqlite2=1]))
  ])

  AC_SUBST(apu_have_sqlite2)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_sqlite2" = "1"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[$sqlite2_LDFLAGS -lsqlite])
    APR_ADDTO(APRUTIL_LIBS,[$sqlite2_LDFLAGS -lsqlite])
  fi

  LIBS="$old_libs"
  CPPFLAGS="$old_cppflags"
  LDFLAGS="$old_ldflags"
])
dnl

