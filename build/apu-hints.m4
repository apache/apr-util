dnl -----------------------------------------------------------------
dnl apu-hints.m4: apr-util's autoconf macros for platform-specific hints
dnl
dnl  We preload various configure settings depending
dnl  on previously obtained platform knowledge.
dnl  We allow all settings to be overridden from
dnl  the command-line.

dnl
dnl APU_PRELOAD
dnl
dnl  Preload various build parameters based on outside knowledge.
dnl
AC_DEFUN(APU_PRELOAD, [
if test "x$apu_preload_done" != "xyes" ; then
    apu_preload_done="yes"

    echo "Applying apr-util hints file rules for $host"

    case "$host" in
        *-ibm-aix*)
        APR_SETIFNULL(apu_iconv_inbuf_const, [1])
        ;;
    *-solaris2*)
        APR_SETIFNULL(apu_iconv_inbuf_const, [1])
        ;;
    esac

fi
])


