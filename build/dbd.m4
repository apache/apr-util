dnl
dnl DBD module
dnl

dnl
dnl APU_CHECK_DBD: compile backends for apr_dbd.
dnl
AC_DEFUN(APU_CHECK_DBD, [
  apu_have_pgsql=0

  AC_ARG_WITH([pgsql], [
  --with-pgsql=DIR          specify PostgreSQL location
  ], [
    apu_have_pgsql=0
    if test "$withval" = "yes"; then
      AC_CHECK_HEADER(libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
      if test "$apu_have_pgsql" == "0"; then
        AC_CHECK_HEADER(postgresql/libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
        if test "$apu_have_pgsql" != "0"; then
          APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include/postgresql])
        fi
      fi
    elif test "$withval" = "no"; then
      apu_have_pgsql=0
    else
      CPPFLAGS="-I$withval/include"
      LIBS="-L$withval/lib "

      AC_MSG_NOTICE(checking for pgsql in $withval)
      AC_CHECK_HEADER(libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
      if test "$apu_have_pgsql" != "0"; then
        APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
      fi
      if test "$apu_have_pgsql" != "1"; then
        AC_CHECK_HEADER(postgresql/libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
        if test "$apu_have_pgsql" != "0"; then
          APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include/postgresql])
          APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        fi
      fi
    fi
  ], [
    apu_have_pgsql=0
    AC_CHECK_HEADER(libpq-fe.h, AC_CHECK_LIB(pq, PQsendQueryPrepared, [apu_have_pgsql=1]))
  ])
  AC_SUBST(apu_have_pgsql)
  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_pgsql" = "1"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[-lpq])
    APR_ADDTO(APRUTIL_LIBS,[-lpq])
  fi
])
dnl
AC_DEFUN(APU_CHECK_DBD_MYSQL, [
  apu_have_mysql=0

  AC_ARG_WITH([mysql], [
  --with-mysql=DIR          **** SEE INSTALL.MySQL ****
  ], [
    apu_have_mysql=0
    if test "$withval" = "yes"; then
      AC_CHECK_HEADER(mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
      if test "$apu_have_mysql" == "0"; then
        AC_CHECK_HEADER(mysql/mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
        if test "$apu_have_mysql" != "0"; then
          APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include/myql])
        fi
      fi
    elif test "$withval" = "no"; then
      apu_have_mysql=0
    else
      CPPFLAGS="-I$withval/include"
      LIBS="-L$withval/lib "

      AC_MSG_NOTICE(checking for mysql in $withval)
      AC_CHECK_HEADER(mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
      if test "$apu_have_mysql" != "0"; then
        APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
      fi

      if test "$apu_have_mysql" != "1"; then
        AC_CHECK_HEADER(mysql/mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
        if test "$apu_have_mysql" != "0"; then
          APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include/mysql])
          APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
        fi
      fi
    fi
  ], [
    apu_have_mysql=0
    AC_CHECK_HEADER(mysql.h, AC_CHECK_LIB(mysqlclient_r, mysql_init, [apu_have_mysql=1]))
  ])

  AC_SUBST(apu_have_mysql)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_mysql" = "1"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[-lmysqlclient_r])
    APR_ADDTO(APRUTIL_LIBS,[-lmysqlclient_r])
  fi
])

