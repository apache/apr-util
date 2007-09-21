dnl -------------------------------------------------------- -*- autoconf -*-
dnl Copyright 2005 The Apache Software Foundation or its licensors, as
dnl applicable.
dnl
dnl Licensed under the Apache License, Version 2.0 (the "License");
dnl you may not use this file except in compliance with the License.
dnl You may obtain a copy of the License at
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

  AC_ARG_WITH([pgsql], APR_HELP_STRING([--with-pgsql=DIR], [specify PostgreSQL location]),
  [
    apu_have_pgsql=0
    if test "$withval" = "yes"; then
      AC_CHECK_HEADERS(libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
      if test "$apu_have_pgsql" = "0"; then
        AC_CHECK_HEADERS(postgresql/libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
      fi
    elif test "$withval" = "no"; then
      apu_have_pgsql=0
    else
      old_cppflags="$CPPFLAGS"
      old_ldflags="$LDFLAGS"

      pgsql_CPPFLAGS="-I$withval/include"
      pgsql_LDFLAGS="-L$withval/lib "

      APR_ADDTO(CPPFLAGS, [$pgsql_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$pgsql_LDFLAGS])

      AC_MSG_NOTICE(checking for pgsql in $withval)
      AC_CHECK_HEADERS(libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
      if test "$apu_have_pgsql" != "0"; then
        APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
      fi
      if test "$apu_have_pgsql" != "1"; then
        AC_CHECK_HEADERS(postgresql/libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
        if test "$apu_have_pgsql" != "0"; then
          APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include/postgresql])
          APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        fi
      fi

      CPPFLAGS="$old_cppflags"
      LDFLAGS="$old_ldflags"
    fi
  ], [
    apu_have_pgsql=0
    AC_CHECK_HEADERS(libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
  ])
  AC_SUBST(apu_have_pgsql)
  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_pgsql" = "1"; then
    LDADD_dbd_pgsql=-lpq
  fi
  AC_SUBST(LDADD_dbd_pgsql)
])
dnl
AC_DEFUN([APU_CHECK_DBD_MYSQL], [
  apu_have_mysql=0

  AC_ARG_WITH([mysql], APR_HELP_STRING([--with-mysql=DIR], [specify MySQL location]),
  [
    apu_have_mysql=0
    if test "$withval" = "yes"; then
      old_cppflags="$CPPFLAGS"
      old_ldflags="$LDFLAGS"

      AC_PATH_PROG([MYSQL_CONFIG],[mysql_config])
      if test "x$MYSQL_CONFIG" != 'x'; then
        mysql_CPPFLAGS="`$MYSQL_CONFIG --include`"
        mysql_LDFLAGS="`$MYSQL_CONFIG --libs_r`"

        APR_ADDTO(CPPFLAGS, [$mysql_CPPFLAGS])
        APR_ADDTO(LDFLAGS, [$mysql_LDFLAGS])
      fi

      AC_CHECK_HEADERS(mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
      if test "$apu_have_mysql" = "0"; then
        AC_CHECK_HEADERS(mysql/mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
      else
        if test "x$MYSQL_CONFIG" != 'x'; then
          APR_ADDTO(APRUTIL_INCLUDES, [$mysql_CPPFLAGS])
        fi
      fi

      CPPFLAGS="$old_cppflags"
      LDFLAGS="$old_ldflags"
    elif test "$withval" = "no"; then
      apu_have_mysql=0
    else
      old_cppflags="$CPPFLAGS"
      old_ldflags="$LDFLAGS"

      AC_PATH_PROG([MYSQL_CONFIG],[mysql_config],,[$withval/bin])
      if test "x$MYSQL_CONFIG" != 'x'; then
        mysql_CPPFLAGS="`$MYSQL_CONFIG --include`"
        mysql_LDFLAGS="`$MYSQL_CONFIG --libs_r`"
      else
        mysql_CPPFLAGS="-I$withval/include"
        mysql_LDFLAGS="-L$withval/lib "
      fi

      APR_ADDTO(CPPFLAGS, [$mysql_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$mysql_LDFLAGS])

      AC_MSG_NOTICE(checking for mysql in $withval)
      AC_CHECK_HEADERS(mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
      if test "$apu_have_mysql" != "0"; then
        APR_ADDTO(APRUTIL_INCLUDES, [$mysql_CPPFLAGS])
      fi

      if test "$apu_have_mysql" != "1"; then
        AC_CHECK_HEADERS(mysql/mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
        if test "$apu_have_mysql" != "0"; then
          APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include/mysql])
        fi
      fi

      CPPFLAGS="$old_cppflags"
      LDFLAGS="$old_ldflags"
    fi
  ], [
    apu_have_mysql=0

    old_cppflags="$CPPFLAGS"
    old_ldflags="$LDFLAGS"

    AC_PATH_PROG([MYSQL_CONFIG],[mysql_config])
    if test "x$MYSQL_CONFIG" != 'x'; then
      mysql_CPPFLAGS="`$MYSQL_CONFIG --include`"
      mysql_LDFLAGS="`$MYSQL_CONFIG --libs_r`"

      APR_ADDTO(CPPFLAGS, [$mysql_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$mysql_LDFLAGS])
    fi

    AC_CHECK_HEADERS(mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))

    if test "$apu_have_mysql" != "0"; then
      if test "x$MYSQL_CONFIG" != 'x'; then
        APR_ADDTO(APRUTIL_INCLUDES, [$mysql_CPPFLAGS])
      fi
    fi

    CPPFLAGS="$old_cppflags"
    LDFLAGS="$old_ldflags"
  ])

  AC_SUBST(apu_have_mysql)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_mysql" = "1"; then
    LDADD_dbd_mysql=$mysql_LDFLAGS
  fi
  AC_SUBST(LDADD_dbd_mysql)
])
dnl
AC_DEFUN([APU_CHECK_DBD_SQLITE3], [
  apu_have_sqlite3=0

  AC_ARG_WITH([sqlite3], APR_HELP_STRING([--with-sqlite3=DIR], [enable sqlite3 DBD driver]),
  [
    apu_have_sqlite3=0
    if test "$withval" = "yes"; then
      AC_CHECK_HEADERS(sqlite3.h, AC_CHECK_LIB(sqlite3, sqlite3_open, [apu_have_sqlite3=1]))
    elif test "$withval" = "no"; then
      apu_have_sqlite3=0
    else
      old_cppflags="$CPPFLAGS"
      old_ldflags="$LDFLAGS"

      sqlite3_CPPFLAGS="-I$withval/include"
      sqlite3_LDFLAGS="-L$withval/lib "

      APR_ADDTO(CPPFLAGS, [$sqlite3_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$sqlite3_LDFLAGS])

      AC_MSG_NOTICE(checking for sqlite3 in $withval)
      AC_CHECK_HEADERS(sqlite3.h, AC_CHECK_LIB(sqlite3, sqlite3_open, [apu_have_sqlite3=1]))
      if test "$apu_have_sqlite3" != "0"; then
        APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
      fi

      CPPFLAGS="$old_cppflags"
      LDFLAGS="$old_ldflags"
    fi
  ], [
    apu_have_sqlite3=0
    AC_CHECK_HEADERS(sqlite3.h, AC_CHECK_LIB(sqlite3, sqlite3_open, [apu_have_sqlite3=1]))
  ])

  AC_SUBST(apu_have_sqlite3)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_sqlite3" = "1"; then
    LDADD_dbd_sqlite3="-lsqlite3"
  fi
  AC_SUBST(LDADD_dbd_sqlite3)
])
dnl
AC_DEFUN([APU_CHECK_DBD_SQLITE2], [
  apu_have_sqlite2=0

  AC_ARG_WITH([sqlite2], APR_HELP_STRING([--with-sqlite2=DIR], [enable sqlite2 DBD driver]),
  [
    apu_have_sqlite2=0
    if test "$withval" = "yes"; then
      AC_CHECK_HEADERS(sqlite.h, AC_CHECK_LIB(sqlite, sqlite_open, [apu_have_sqlite2=1]))
    elif test "$withval" = "no"; then
      apu_have_sqlite2=0
    else
      old_cppflags="$CPPFLAGS"
      old_ldflags="$LDFLAGS"

      sqlite2_CPPFLAGS="-I$withval/include"
      sqlite2_LDFLAGS="-L$withval/lib "

      APR_ADDTO(CPPFLAGS, [$sqlite2_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$sqlite2_LDFLAGS])

      AC_MSG_NOTICE(checking for sqlite2 in $withval)
      AC_CHECK_HEADERS(sqlite.h, AC_CHECK_LIB(sqlite, sqlite_open, [apu_have_sqlite2=1]))
      if test "$apu_have_sqlite2" != "0"; then
        APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
      fi

      CPPFLAGS="$old_cppflags"
      LDFLAGS="$old_ldflags"
    fi
  ], [
    apu_have_sqlite2=0
    AC_CHECK_HEADERS(sqlite.h, AC_CHECK_LIB(sqlite, sqlite_open, [apu_have_sqlite2=1]))
  ])

  AC_SUBST(apu_have_sqlite2)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_sqlite2" = "1"; then
    LDADD_dbd_sqlite2="-lsqlite"
  fi
  AC_SUBST(LDADD_dbd_sqlite2)
])
dnl
AC_DEFUN([APU_CHECK_DBD_ORACLE], [
  apu_have_oracle=0

  AC_ARG_WITH([oracle-include],
    APR_HELP_STRING([--with-oracle-include=DIR], [path to Oracle include files]))
  AC_ARG_WITH([oracle], 
    APR_HELP_STRING([--with-oracle=DIR], [enable Oracle DBD driver; giving ORACLE_HOME as DIR]),
  [
    apu_have_oracle=0
    if test "$withval" = "yes"; then
      old_cppflags="$CPPFLAGS"

      if test -n "$with_oracle_include"; then
        oracle_CPPFLAGS="$CPPFLAGS -I$with_oracle_include"
        APR_ADDTO(APRUTIL_INCLUDES, [-I$with_oracle_include])
      fi

      APR_ADDTO(CPPFLAGS, [$oracle_CPPFLAGS])

      AC_CHECK_HEADERS(oci.h, AC_CHECK_LIB(clntsh, OCIEnvCreate, [apu_have_oracle=1],[
        unset ac_cv_lib_clntsh_OCIEnvCreate
        AC_CHECK_LIB(clntsh, OCIEnvCreate, [
          apu_have_oracle=1
          LDADD_dbd_oracle="-lnnz10"
        ],,[-lnnz10])
      ]))

      CPPFLAGS="$old_cppflags"
    elif test "$withval" = "no"; then
      apu_have_oracle=0
    else
      old_cppflags="$CPPFLAGS"
      old_ldflags="$LDFLAGS"

      if test -n "$with_oracle_include"; then
        oracle_CPPFLAGS="$CPPFLAGS -I$with_oracle_include"
        APR_ADDTO(APRUTIL_INCLUDES, [-I$with_oracle_include])
      else
        oracle_CPPFLAGS="-I$withval/rdbms/demo -I$withval/rdbms/public"
      fi
      oracle_LDFLAGS="-L$withval/lib "

      APR_ADDTO(CPPFLAGS, [$oracle_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$oracle_LDFLAGS])

      AC_MSG_NOTICE(checking for oracle in $withval)
      AC_CHECK_HEADERS(oci.h, AC_CHECK_LIB(clntsh, OCIEnvCreate, [apu_have_oracle=1],[
        unset ac_cv_lib_clntsh_OCIEnvCreate
        AC_CHECK_LIB(clntsh, OCIEnvCreate, [
          apu_have_oracle=1
          LDADD_dbd_oracle="-lnnz10"
        ],,[-lnnz10])
      ]))
      if test "$apu_have_oracle" != "0"; then
        APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        APR_ADDTO(APRUTIL_LDFLAGS, [-R$withval/lib])
        if test -z "$with_oracle_include"; then
          APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/rdbms/demo])
          APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/rdbms/public])
        fi
      fi

      CPPFLAGS="$old_cppflags"
      LDFLAGS="$old_ldflags"
    fi
  ], [
    apu_have_oracle=0

    old_cppflags="$CPPFLAGS"

    if test -n "$with_oracle_include"; then
      oracle_CPPFLAGS="$CPPFLAGS -I$with_oracle_include"
      APR_ADDTO(APRUTIL_INCLUDES, [-I$with_oracle_include])
    fi

    APR_ADDTO(CPPFLAGS, [$oracle_CPPFLAGS])

    AC_CHECK_HEADERS(oci.h, AC_CHECK_LIB(clntsh, OCIEnvCreate, [apu_have_oracle=1],[
      unset ac_cv_lib_clntsh_OCIEnvCreate
      AC_CHECK_LIB(clntsh, OCIEnvCreate, [
        apu_have_oracle=1
        LDADD_dbd_oracle=-lnnz10
      ],,[-lnnz10])
    ]))

    CPPFLAGS="$old_cppflags"
  ])

  AC_SUBST(apu_have_oracle)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_oracle" = "1"; then
    LDADD_dbd_oracle="$LDADD_dbd_oracle -lclntsh"
  fi
  AC_SUBST(LDADD_dbd_oracle)
])

dnl
AC_DEFUN([APU_CHECK_DBD_FREETDS], [
  apu_have_freetds=0

  AC_ARG_WITH([freetds], 
    APR_HELP_STRING([--with-freetds=DIR], [specify FreeTDS location]),
  [
    apu_have_freetds=0
    if test "$withval" = "yes"; then
      AC_CHECK_HEADERS(sybdb.h, AC_CHECK_LIB(sybdb, tdsdbopen, [apu_have_freetds=1]))
    elif test "$withval" = "no"; then
      apu_have_freetds=0
    else
      old_cppflags="$CPPFLAGS"
      old_ldflags="$LDFLAGS"

      sybdb_CPPFLAGS="-I$withval/include"
      sybdb_LDFLAGS="-L$withval/lib "

      APR_ADDTO(CPPFLAGS, [$sybdb_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$sybdb_LDFLAGS])

      AC_MSG_NOTICE(checking for freetds in $withval)
      AC_CHECK_HEADERS(sybdb.h, AC_CHECK_LIB(sybdb, tdsdbopen, [apu_have_freetds=1]))
      if test "$apu_have_freetds" != "0"; then
        APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
      fi

      CPPFLAGS="$old_cppflags"
      LDFLAGS="$old_ldflags"
    fi
  ], [
    apu_have_freetds=0
    AC_CHECK_HEADERS(sybdb.h, AC_CHECK_LIB(sybdb, tdsdbopen, [apu_have_freetds=1]))
  ])

  AC_SUBST(apu_have_freetds)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_freetds" = "1"; then
    LDADD_dbd_freetds="$LDADD_dbd_freetds -lsybdb"
    dnl Erm, I needed pcreposix, but I think that dependency has gone
    dnl from the current code
    dnl LDADD_dbd_freetds="$LDADD_dbd_freetds -lsybdb -lpcreposix"
  fi
  AC_SUBST(LDADD_dbd_freetds)
])
dnl

AC_DEFUN([APU_CHECK_DBD_DSO], [

  AC_ARG_ENABLE([dbd-dso], 
     APR_HELP_STRING([--enable-dbd-dso], [build DBD drivers as DSOs]))

  if test "$enable_dbd_dso" = "yes"; then
     AC_DEFINE([APU_DSO_BUILD], 1, [Define if DBD drivers are built as DSOs])
     
     dsos=
     test $apu_have_oracle = 1 && dsos="$dsos dbd/apr_dbd_oracle.la"
     test $apu_have_pgsql = 1 && dsos="$dsos dbd/apr_dbd_pgsql.la"
     test $apu_have_mysql = 1 && dsos="$dsos dbd/apr_dbd_mysql.la"
     test $apu_have_sqlite2 = 1 && dsos="$dsos dbd/apr_dbd_sqlite2.la"
     test $apu_have_sqlite3 = 1 && dsos="$dsos dbd/apr_dbd_sqlite3.la"
     test $apu_have_freetds = 1 && dsos="$dsos dbd/apr_dbd_freetds.la"

     APU_MODULES="$APU_MODULES $dsos"
  else
     # Statically link the DBD drivers:

     objs=
     test $apu_have_oracle = 1 && objs="$objs dbd/apr_dbd_oracle.lo"
     test $apu_have_pgsql = 1 && objs="$objs dbd/apr_dbd_pgsql.lo"
     test $apu_have_mysql = 1 && objs="$objs dbd/apr_dbd_mysql.lo"
     test $apu_have_sqlite2 = 1 && objs="$objs dbd/apr_dbd_sqlite2.lo"
     test $apu_have_sqlite3 = 1 && objs="$objs dbd/apr_dbd_sqlite3.lo"
     test $apu_have_freetds = 1 && objs="$objs dbd/apr_dbd_freetds.lo"
     EXTRA_OBJECTS="$EXTRA_OBJECTS $objs"

     # Use libtool *.la for mysql if available
     if test $apu_have_mysql = 1; then
       for flag in $LDADD_dbd_mysql
       do
         dir=`echo $flag | grep "^-L" | sed s:-L::`
         if test "x$dir" != 'x'; then
           if test -f "$dir/libmysqlclient_r.la"; then
             LDADD_dbd_mysql=$dir/libmysqlclient_r.la
             break
           fi
         fi
       done
     fi

     APRUTIL_LIBS="$APRUTIL_LIBS $LDADD_dbd_pgsql $LDADD_dbd_sqlite2 $LDADD_dbd_sqlite3 $LDADD_dbd_oracle $LDADD_dbd_mysql $LDADD_dbd_freetds"
     APRUTIL_EXPORT_LIBS="$APRUTIL_EXPORT_LIBS $LDADD_dbd_pgsql $LDADD_dbd_sqlite2 $LDADD_dbd_sqlite3 $LDADD_dbd_oracle $LDADD_dbd_mysql $LDADD_dbd_freetds"
  fi
])
