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
dnl APU_CHECK_DBM: see what kind of DBM backend to use for apr_dbm.
dnl
AC_DEFUN(APU_CHECK_DBM,[

apu_use_sdbm=0
apu_use_gdbm=0
AC_MSG_CHECKING(for chosen DBM type)
AC_ARG_WITH(dbm,
  [  --with-dbm=DBM          choose the DBM type to use. DBM={sdbm,gdbm}],[
  if test "$withval" = "yes"; then
    AC_MSG_ERROR([You need to specify a DBM type to use. One of: sdbm, gdbm])
  fi
  case "$withval" in
    sdbm)
      apu_use_sdbm=1
      AC_MSG_RESULT(sdbm)
      ;;
    gdbm)
      apu_use_gdbm=1
      AC_MSG_RESULT(gdbm)
      ;;
    *)
      AC_MSG_ERROR([$withval is an unknown DBM type. Use one of: sdbm, gdbm])
      ;;
  esac
],[
  apu_use_sdbm=1
  AC_MSG_RESULT([sdbm (default)])
])
AC_SUBST(apu_use_sdbm)
AC_SUBST(apu_use_gdbm)

if test $apu_use_gdbm = 1; then
  lib_save="$LIBS"
  LIBS=""
  AC_CHECK_LIB(gdbm, gdbm_open)
  APRUTIL_EXPORT_LIBS="$APRUTIL_EXPORT_LIBS $LIBS"
  LIBS="$lib_save $LIBS"
fi

])
