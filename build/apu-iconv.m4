dnl
dnl custom autoconf rules for APRUTIL
dnl

dnl
dnl APU_TRY_ICONV[ IF-SUCCESS, IF-FAILURE ]: try to compile for iconv.
dnl
AC_DEFUN(APU_TRY_ICONV,[
  AC_TRY_LINK([
#include <stdlib.h>
#include <iconv.h>
],
[
  iconv_t cd = iconv_open("", "");
  iconv(cd, NULL, NULL, NULL, NULL);
], [$1], [$2])
])

dnl
dnl APU_FIND_ICONV: find an iconv library
dnl
AC_DEFUN(APU_FIND_ICONV,[

apu_iconv_dir="unknown"
have_apr_iconv="0"
AC_ARG_WITH(iconv,[  --with-iconv[=DIR]        path to iconv installation],
  [ apu_iconv_dir="$withval"
    if test "$apu_iconv_dir" != "yes"; then
      APR_ADDTO(CPPFLAGS,[-I$apu_iconv_dir/include])
      APR_ADDTO(LDFLAGS,[-L$apu_iconv_dir/lib])
    fi
    if test -f "$apu_iconv_dir/include/api_version.h"; then
      have_apr_iconv="1"
      have_iconv="0"
      APR_REMOVEFROM(LIBS,[-lapriconv])
      AC_MSG_RESULT("Using apr-iconv")
    fi
  ])

if test "$have_apr_iconv" != "1"; then
  AC_CHECK_HEADER(iconv.h, [
    APU_TRY_ICONV([ have_iconv="1" ], [

    APR_ADDTO(LIBS,[-liconv])

    APU_TRY_ICONV([
      APR_ADDTO(APRUTIL_LIBS,[-liconv])
      APR_ADDTO(APRUTIL_EXPORT_LIBS,[-liconv])
      have_iconv="1" ],
      [ have_iconv="0" ])

    APR_REMOVEFROM(LIBS,[-liconv])

    ])
  ], [ have_iconv="0" ])
fi

if test "$apu_iconv_dir" != "unknown"; then
  if test "$have_iconv" != "1"; then
    if test "$have_apr_iconv" != "1"; then 
      AC_MSG_ERROR([iconv support requested, but not found])
    fi
  fi
  APR_REMOVEFROM(CPPFLAGS,[-I$apu_iconv_dir/include])
  APR_REMOVEFROM(LDFLAGS,[-L$apu_iconv_dir/lib])
  APR_ADDTO(APRUTIL_INCLUDES,[-I$apu_iconv_dir/include])
  APR_ADDTO(APRUTIL_LDFLAGS,[-L$apu_iconv_dir/lib])
fi

if test "$have_iconv" = "1"; then
  APU_CHECK_ICONV_INBUF
fi

APR_FLAG_HEADERS(iconv.h langinfo.h)
APR_FLAG_FUNCS(nl_langinfo)
APR_CHECK_DEFINE(CODESET, langinfo.h, [CODESET defined in langinfo.h])

AC_SUBST(have_iconv)
AC_SUBST(have_apr_iconv)
])dnl

dnl
dnl APU_CHECK_ICONV_INBUF
dnl
dnl  Decide whether or not the inbuf parameter to iconv() is const.
dnl
dnl  We try to compile something without const.  If it fails to 
dnl  compile, we assume that the system's iconv() has const.  
dnl  Unfortunately, we won't realize when there was a compile
dnl  warning, so we allow a variable -- apu_iconv_inbuf_const -- to
dnl  be set in hints.m4 to specify whether or not iconv() has const
dnl  on this parameter.
dnl
AC_DEFUN(APU_CHECK_ICONV_INBUF,[
AC_MSG_CHECKING(for type of inbuf parameter to iconv)
if test "x$apu_iconv_inbuf_const" = "x"; then
    APR_TRY_COMPILE_NO_WARNING([
    #include <stddef.h>
    #include <iconv.h>
    ],[
    iconv(0,(char **)0,(size_t *)0,(char **)0,(size_t *)0);
    ], apu_iconv_inbuf_const="0", apu_iconv_inbuf_const="1")
fi
if test "$apu_iconv_inbuf_const" = "1"; then
    AC_DEFINE(APU_ICONV_INBUF_CONST, 1, [Define if the inbuf parm to iconv() is const char **])
    msg="const char **"
else
    msg="char **"
fi
AC_MSG_RESULT([$msg])
])dnl
