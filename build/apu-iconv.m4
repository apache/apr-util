dnl
dnl custom autoconf rules for APRUTIL
dnl

dnl
dnl APU_FIND_ICONV: find an iconv library
dnl
AC_DEFUN(APU_FIND_ICONV,[

dnl
dnl TODO: Check for --with-iconv or --with-apr-iconv, or look for
dnl apr-iconv sources or an installed apr-iconv ...
dnl

AC_CHECK_LIB(iconv, iconv)
AC_CHECK_FUNCS(iconv, [ have_iconv="1" ], [ have_iconv="0" ])
if test "$have_iconv" = "1"; then
  APU_CHECK_ICONV_INBUF
fi
AC_SUBST(have_iconv)
APR_FLAG_HEADERS(iconv.h)

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
    AC_DEFINE(APR_ICONV_INBUF_CONST, 1, [Define if the inbuf parm to iconv() is const char **])
    msg="const char **"
else
    msg="char **"
fi
AC_MSG_RESULT([$msg])
])dnl
