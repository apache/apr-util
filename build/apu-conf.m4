dnl
dnl custom autoconf rules for APRUTIL
dnl

dnl
dnl APU_FIND_APR: figure out where APR is located
dnl
AC_DEFUN(APU_FIND_APR,[

AC_MSG_CHECKING(for APR)
AC_ARG_WITH(apr,
[  --with-apr=DIR          path to APR source or the APR includes],
[
    if test "$withval" = "yes"; then
        AC_MSG_ERROR(You need to specify a directory with --with-apr)
    fi
    absdir="`cd $withval ; pwd`"
    if test -f "$absdir/apr_pools.h"; then
	APR_INCLUDES="$absdir"
    elif test -f "$absdir/include/apr_pools.h"; then
	APR_SOURCE_DIR="$absdir"
    fi
],[
    dnl see if we can find APR
    if test -f "$srcdir/apr/include/apr_pools.h"; then
	APR_SOURCE_DIR="$srcdir/apr"
    elif test -f "$srcdir/../apr/include/apr_pools.h"; then
	APR_SOURCE_DIR="`cd $srcdir/../apr ; pwd`"
    fi
])
if test -n "$APR_SOURCE_DIR"; then
    APR_INCLUDES="$APR_SOURCE_DIR/include"
fi
if test -z "$APR_INCLUDES"; then
    AC_MSG_RESULT(not found)
    AC_MSG_ERROR(APR could not be located. Please use the --with-apr option.)
fi
AC_MSG_RESULT($APR_INCLUDES)

AC_SUBST(APR_SOURCE_DIR)
])

dnl
dnl APU_CHECK_DB1: is DB1 present?
dnl
dnl if present: sets apu_use_db=1, db_header, and db_lib
dnl
AC_DEFUN(APU_CHECK_DB1,[
AC_CHECK_HEADER(db1/db.h, [
  apu_use_db=1
  db_header=db1/db.h
  db_lib=db1
  ])
])

dnl
dnl APU_CHECK_DB185: is DB1.85 present?
dnl
dnl if present: sets apu_use_db=1, db_header, and db_lib
dnl
AC_DEFUN(APU_CHECK_DB185,[
AC_CHECK_HEADER(db_185.h, [
  apu_use_db=1
  db_header=db_185.h
  db_lib=db1
  ])
])

dnl
dnl APU_CHECK_DB2or3: are DB2 or DB3 present?
dnl
dnl if present: sets apu_use_db=1, db_header, and db_lib
dnl
AC_DEFUN(APU_CHECK_DB2or3,[
AC_CHECK_HEADER(db.h, [
  apu_use_db=1
  db_header=db.h
  db_lib=db
  ])
])

dnl
dnl APU_CHECK_DB_VSN: check the actual version of db (for db2 or db3)
dnl
dnl sets db_version
dnl
AC_DEFUN(APU_CHECK_DB_VSN,[
  apu_save_libs="$LIBS"
  LIBS="$LIBS -ldb"
  AC_TRY_RUN([
#include "db.h"
main()
{
    int major, minor, patch;
    db_version(&major, &minor, &patch);
    if (major == 2)
        exit(1);
    exit(0);
}
], db_version=3, db_version=2, db_version=2)
  LIBS="$apu_save_libs"
])

dnl
dnl APU_FIND_DB: find one of the DB libraries to use
dnl
dnl if found, then which_dbm is set to one of: db1, db185, db2, db3
dnl
AC_DEFUN(APU_FIND_DB,[
  APU_CHECK_DB2or3
  if test $apu_use_db = 1; then
    APU_CHECK_DB_VSN
    which_dbm="db$db_version"
  else
    APU_CHECK_DB1
    if test $apu_use_db = 1; then
      which_dbm="db1"
    else
      APU_CHECK_DB185
      if test $apu_use_db = 1; then
        which_dbm="db185"
      fi
    fi
  fi
])

dnl
dnl APU_CHECK_DBM: see what kind of DBM backend to use for apr_dbm.
dnl
AC_DEFUN(APU_CHECK_DBM,[

apu_use_sdbm=0
apu_use_gdbm=0
apu_use_db=0
db_header=db.h		# default so apu_select_dbm.h is syntactically correct

AC_ARG_WITH(dbm,
  [  --with-dbm=DBM          choose the DBM type to use.
                          DBM={sdbm,gdbm,db,db1,db185,db2,db3}],[
  if test "$withval" = "yes"; then
    AC_MSG_ERROR([--with-dbm needs to specify a DBM type to use.
One of: sdbm, gdbm, db, db1, db185, db2, db3])
  fi
  look_for="$withval"
],[
  look_for=default
])

case "$look_for" in
  sdbm)
    apu_use_sdbm=1
    which_dbm=sdbm
    ;;
  gdbm)
    apu_use_gdbm=1
    which_dbm=gdbm
    ;;
  db)
    APU_FIND_DB
    if test -n "$which_dbm"; then
      # pretend we were looking for this one
      look_for=$which_dbm
    else
      look_errmsg="could not find a DB header"
    fi
    ;;
  db1)
    APU_CHECK_DB1
    if test $apu_use_db = 1; then
      which_dbm=db1
    fi
    ;;
  db185)
    APU_CHECK_DB185
    if test $apu_use_db = 1; then
      which_dbm=db185
    fi
    ;;
  db2)
    APU_CHECK_DB2or3
    if test $apu_use_db = 1; then
      APU_CHECK_DB_VSN
      if test "$db_version" = 2; then
        which_dbm=db2
      else
        look_errmsg="db2 not present (found db3)"
      fi
    fi
    ;;
  db3)
    APU_CHECK_DB2or3
    if test $apu_use_db = 1; then
      APU_CHECK_DB_VSN
      if test "$db_version" = 3; then
        which_dbm=db3
      else
        look_errmsg="db3 not present (found db2)"
      fi
    fi
    ;;
  default)
    dnl ### use more sophisticated DBMs for the default?
    which_dbm="sdbm (default)"
    apu_use_sdbm=1
    ;;
  *)
    look_errmsg="--with-dbm=$look_for is an unknown DBM type.
Use one of: sdbm, gdbm, db, db1, db185, db2, db3"
    ;;
esac

AC_MSG_CHECKING(for chosen DBM type)
if test "$look_for" = "$which_dbm"; then
  AC_MSG_RESULT($which_dbm)
elif test "$look_for" = "default"; then
  AC_MSG_RESULT($which_dbm)
elif test -n "$look_errmsg"; then
  AC_MSG_ERROR($look_errmsg)
else
  AC_MSG_ERROR($look_for not present)
fi

AC_SUBST(apu_use_sdbm)
AC_SUBST(apu_use_gdbm)
AC_SUBST(apu_use_db)
AC_SUBST(db_header)

if test $apu_use_gdbm = 1; then
  lib_save="$LIBS"
  LIBS=""
  AC_CHECK_LIB(gdbm, gdbm_open)
  APRUTIL_EXPORT_LIBS="$APRUTIL_EXPORT_LIBS $LIBS"
  LIBS="$lib_save $LIBS"
fi

if test $apu_use_db = 1; then
  dnl ### use AC_CHECK_LIB?
  LIBS="$LIBS -l$db_lib"
  APRUTIL_EXPORT_LIBS="$APRUTIL_EXPORT_LIBS $LIBS"
fi

])
