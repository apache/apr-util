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
    if test -f "$absdir/apr.h"; then
	APR_INCLUDES="$absdir"
    elif test -f "$absdir/include/apr.h"; then
	APR_BUILD_DIR="$absdir"
    fi
],[
    dnl see if we can find APR
    if test -f "$srcdir/apr/include/apr.h"; then
	APR_BUILD_DIR="$srcdir/apr"
    elif test -f "$srcdir/../apr/include/apr.h"; then
	APR_BUILD_DIR="`cd $srcdir/../apr ; pwd`"
    fi
])

dnl
dnl grab flags from APR.
dnl ### APR doesn't have "nice" names for its exports (yet), but it isn't
dnl ### a problem to deal with them right here
dnl

. "$APR_BUILD_DIR/APRVARS"
APR_EXPORT_CPPFLAGS="$EXTRA_CPPFLAGS"
APR_EXPORT_CFLAGS="$EXTRA_CFLAGS"
APR_EXPORT_LIBS="$EXTRA_LIBS"

if test -n "$APR_BUILD_DIR"; then
    APR_INCLUDES="-I$APR_BUILD_DIR/include"
    if test "$APR_BUILD_DIR" != "$APR_SOURCE_DIR"; then
        APR_INCLUDES="$APR_INCLUDES -I$APR_SOURCE_DIR/include"
    fi
fi

if test -z "$APR_INCLUDES"; then
    AC_MSG_RESULT(not found)
    AC_MSG_ERROR(APR could not be located. Please use the --with-apr option.)
fi

AC_MSG_RESULT($APR_INCLUDES)

AC_SUBST(APR_BUILD_DIR)
AC_SUBST(APR_SOURCE_DIR)
])

dnl
dnl APU_CHECK_DB1: is DB1 present?
dnl
dnl if present: sets apu_have_db=1, db_header, and db_lib
dnl
AC_DEFUN(APU_CHECK_DB1,[
AC_CHECK_HEADER(db1/db.h, [
  apu_have_db=1
  db_header=db1/db.h
  db_lib=db1
  ])
])

dnl
dnl APU_CHECK_DB185: is DB1.85 present?
dnl
dnl if present: sets apu_have_db=1, db_header, and db_lib
dnl
AC_DEFUN(APU_CHECK_DB185,[
AC_CHECK_HEADER(db_185.h, [
  apu_have_db=1
  db_header=db_185.h
  db_lib=db1
  ])
])

dnl
dnl APU_CHECK_DB2or3: are DB2 or DB3 present?
dnl
dnl if present: sets apu_have_db=1, db_header, and db_lib
dnl
AC_DEFUN(APU_CHECK_DB2or3,[
AC_CHECK_HEADER(db.h, [
  apu_have_db=1
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
  if test $apu_have_db = 1; then
    APU_CHECK_DB_VSN
    which_dbm="db$db_version"
  else
    APU_CHECK_DB1
    if test $apu_have_db = 1; then
      which_dbm="db1"
    else
      APU_CHECK_DB185
      if test $apu_have_db = 1; then
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
dnl it's in our codebase
apu_have_sdbm=1
apu_have_gdbm=0
apu_have_db=0

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

AC_CHECK_LIB( gdbm, gdbm_open, 
    [ AC_CHECK_HEADER( gdbm.h, 
        apu_have_gdbm=1,
        apu_have_gdbm=0)],
      AC_MSG_WARN( "gdbm DBM not found"),)

APU_FIND_DB

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
    if test $apu_have_db = 1; then
      apu_use_db=1
      which_dbm=db
    else
     look_errmsg="couldn't find berkley DB"
    fi
    ;;
  db1)
    apu_have_db=0
    APU_CHECK_DB1
    if test $apu_have_db = 1; then
      which_dbm=db1
      apu_use_db=1
    fi
    ;;
  db185)
    apu_have_db=0
    APU_CHECK_DB185
    if test $apu_have_db = 1; then
      which_dbm=db185
      apu_use_db=1
    fi
    ;;
  db2)
    apu_have_db=0
    APU_CHECK_DB2or3
    if test $apu_have_db = 1; then
      apu_use_db=1
      APU_CHECK_DB_VSN
      if test "$db_version" = 2; then
        which_dbm=db2
      else
        look_errmsg="db2 not present (found db3)"
      fi
    fi
    ;;
  db3)
    apu_have_db=0
    APU_CHECK_DB2or3
    if test $apu_have_db = 1; then
      apu_use_db=1
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

AC_SUBST(apu_have_sdbm)
AC_SUBST(apu_have_gdbm)
AC_SUBST(apu_have_db)
AC_SUBST(db_header)
AC_SUBST(db_version)

if test $apu_have_gdbm = 1; then
  lib_save="$LIBS"
  LIBS=""
  AC_CHECK_LIB(gdbm, gdbm_open)
  APRUTIL_EXPORT_LIBS="$APRUTIL_EXPORT_LIBS $LIBS"
  LIBS="$lib_save $LIBS"
fi

if test $apu_have_db = 1; then
  dnl ### use AC_CHECK_LIB?
  LIBS="$LIBS -l$db_lib"
  APRUTIL_EXPORT_LIBS="$APRUTIL_EXPORT_LIBS -l$db_lib"
fi

])

dnl APU_SUBDIR_CONFIG
dnl ### we should use APR's copy of this
AC_DEFUN(APU_SUBDIR_CONFIG, [
  # save our work to this point; this allows the sub-package to use it
  AC_CACHE_SAVE

  echo "configuring package in $1 now"
  ac_popdir=`pwd`
  ac_abs_srcdir=`(cd $srcdir/$1 && pwd)`
  apr_config_subdirs="$1"
  test -d $1 || $MKDIR $1
  cd $1

changequote(, )dnl
      # A "../" for each directory in /$config_subdirs.
      ac_dots=`echo $apr_config_subdirs|sed -e 's%^\./%%' -e 's%[^/]$%&/%' -e 's%[^/]*/%../%g'`
changequote([, ])dnl

  # Make the cache file name correct relative to the subdirectory.
  case "$cache_file" in
  /*) ac_sub_cache_file=$cache_file ;;
  *) # Relative path.
    ac_sub_cache_file="$ac_dots$cache_file" ;;
  esac

  # The eval makes quoting arguments work.
  if eval $ac_abs_srcdir/configure $ac_configure_args --cache-file=$ac_sub_cache_file --srcdir=$ac_abs_srcdir $2
  then :
    echo "$1 configured properly"
  else
    echo "configure failed for $1"
    exit 1
  fi

  cd $ac_popdir

  # grab any updates from the sub-package
  AC_CACHE_LOAD
])dnl

dnl
dnl APU_TEST_EXPAT(directory): test if Expat is located in the specified dir
dnl
dnl if present: sets expat_include_dir, expat_libs, possibly expat_old
dnl
AC_DEFUN(APU_TEST_EXPAT,[
  AC_MSG_CHECKING(for Expat in ifelse($2,,$1,$2))

  if test -r "$1/lib/expat.h.in"; then
    dnl Expat 1.95.* distribution
    expat_include_dir="$1/lib"
    expat_libs="$1/lib/libexpat.la"
  elif test -r "$1/xmlparse.h"; then
    dnl maybe an expat-lite. use this dir for both includes and libs
    expat_include_dir="$1"
    expat_libs="$1/libexpat.la"
    expat_old=yes
  elif test -r "$1/include/xmlparse.h" -a \
       -r "$1/lib/libexpat.a"; then
    dnl previously installed expat
    expat_include_dir="$1/include"
    expat_libs="-L$1/lib -lexpat"
    expat_old=yes
  elif test -r "$1/include/xml/xmlparse.h" -a \
       -r "$1/lib/xml/libexpat.a"; then
    dnl previously installed expat
    expat_include_dir="$1/include/xml"
    expat_libs="-L$1/lib -lexpat"
    expat_old=yes
  elif test -r "$1/include/xmltok/xmlparse.h"; then
    dnl Debian distribution
    expat_include_dir="$1/include/xmltok"
    expat_libs="-L$1/lib -lxmlparse -lxmltok"
    expat_old=yes
  elif test -r "$1/xmlparse/xmlparse.h"; then
    dnl Expat 1.0 or 1.1 source directory
    expat_include_dir="$1/xmlparse"
    expat_libs="-L$1 -lexpat"
    expat_old=yes
  fi
  dnl ### test for installed Expat 1.95.* distros

  if test -n "$expat_include_dir"; then
    dnl ### more info about what we found there? version? using .la?
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
])


dnl
dnl APU_FIND_EXPAT: figure out where EXPAT is located (or use bundled)
dnl
AC_DEFUN(APU_FIND_EXPAT,[

AC_ARG_WITH([expat],
[  --with-expat=DIR        specify Expat location], [
  if test "$withval" = "yes"; then
    AC_MSG_ERROR([a directory must be specified for --with-expat])
  elif test "$withval" = "no"; then
    AC_MSG_ERROR([Expat cannot be disabled (at this time)])
  else
    abs_expatdir="`cd $withval && pwd`"
    APU_TEST_EXPAT($abs_expatdir, $withval)
    if test -z "$expat_include_dir"; then
      AC_MSG_ERROR([Expat was not found (or recognized) in \"$withval\"])
    fi
  fi
])

if test -z "$expat_include_dir"; then
  for d in /usr /usr/local xml/expat-cvs xml/expat $srcdir/xml/expat ; do
    APU_TEST_EXPAT($d)
    if test -n "$expat_include_dir"; then
      break
    fi
  done
fi
if test -z "$expat_include_dir"; then
  AC_MSG_ERROR([could not locate Expat. use --with-expat])
fi

if test -n "$expat_old"; then
  AC_DEFINE(APR_HAVE_OLD_EXPAT, 1, [define if Expat 1.0 or 1.1 was found])
fi

dnl special-case the bundled distribution (use absolute dirs)
if test "$expat_include_dir" = "xml/expat/lib" -o "$expat_include_dir" = "xml/expat-cvs/lib"; then
  bundled_subdir="`echo $expat_include_dir | sed -e 's%/lib%%'`"
  APU_SUBDIR_CONFIG($bundled_subdir)
  expat_include_dir=$top_builddir/$bundled_subdir/lib
  expat_libs=$top_builddir/$bundled_subdir/lib/libexpat.la
  APR_XML_SUBDIRS="`echo $bundled_subdir | sed -e 's%xml/%%'`"
else
if test "$expat_include_dir" = "$srcdir/xml/expat/include" -o "$expat_include_dir" = "$srcdir/xml/expat/lib"; then
  dnl This is a bit of a hack.  This only works because we know that
  dnl we are working with the bundled version of the software.
  bundled_subdir="xml/expat"
  APU_SUBDIR_CONFIG($bundled_subdir)
  expat_include_dir=$top_builddir/$bundled_subdir/lib
  expat_libs=$top_builddir/$bundled_subdir/lib/libexpat.la
  APR_XML_SUBDIRS="`echo $bundled_subdir | sed -e 's%xml/%%'`"
fi
fi
APR_XML_DIR=$bundled_subdir
AC_SUBST(APR_XML_SUBDIRS)
AC_SUBST(APR_XML_DIR)

APR_ADDTO(INCLUDES, [-I$expat_include_dir])
APR_ADDTO(LIBS, [$expat_libs])
APR_ADDTO(APRUTIL_EXPORT_LIBS, [$expat_libs])
dnl ### export the Expat includes?
])


dnl 
dnl Find a particular LDAP library
dnl
AC_DEFUN(APU_FIND_LDAPLIB,[
  if test ${apu_has_ldap} != "define"; then
    ldaplib=$1
    extralib=$2
    unset ac_cv_lib_${ldaplib}_ldap_init
    AC_CHECK_LIB(${ldaplib}, ldap_init, 
      [
dnl        APR_ADDTO(CPPFLAGS,[-DAPU_HAS_LDAP])
        APR_ADDTO(LIBS,[-l${ldaplib} ${extralib}])
        APR_ADDTO(APRUTIL_EXPORT_LIBS,[-l${ldaplib} ${extralib}])
        AC_CHECK_LIB(${ldaplib}, ldapssl_install_routines, apu_has_ldap_netscape_ssl="define", , ${extralib})
        AC_CHECK_LIB(${ldaplib}, ldap_start_tls_s, apu_has_ldap_starttls="define", , ${extralib})
        apu_has_ldap="define";
      ], , ${extralib})
  fi
])


dnl
dnl APU_FIND_LDAP: figure out where LDAP is located
dnl
AC_DEFUN(APU_FIND_LDAP,[

echo $ac_n "${nl}checking for ldap support..."

apu_has_ldap="undef";
apu_has_ldap_netscape_ssl="undef"
apu_has_ldap_starttls="undef"

AC_ARG_WITH(ldap-include,[  --with-ldap-include=path  path to ldap include files with trailing slash])
AC_ARG_WITH(ldap-lib,[  --with-ldap-lib=path    path to ldap lib file])
AC_ARG_WITH(ldap,[  --with-ldap=library     ldap library to use],
  [
    if test -n "$with_ldap_include"; then
      APR_ADDTO(CPPFLAGS, [-I$with_ldap_include])
    fi
    if test -n "$with_ldap_lib"; then
      APR_ADDTO(LDFLAGS, [-L$with_ldap_lib])
    fi

    LIBLDAP="$withval"
    if test "$LIBLDAP" = "yes"; then
dnl The iPlanet C SDK 5.0 is as yet untested... 
      APU_FIND_LDAPLIB("ldap50", "-lnspr4 -lplc4 -lplds4 -liutil50 -llber50 -lldif50 -lnss3 -lprldap50 -lssl3 -lssldap50")
      APU_FIND_LDAPLIB("ldapssl41", "-lnspr3 -lplc3 -lplds3")
      APU_FIND_LDAPLIB("ldapssl40")
      APU_FIND_LDAPLIB("ldapssl30")
      APU_FIND_LDAPLIB("ldapssl20")
      APU_FIND_LDAPLIB("ldap", "-llber")
    else
      APU_FIND_LDAPLIB($LDAPLIB)
    fi

    test ${apu_has_ldap} != "define" && AC_MSG_ERROR(could not find an LDAP library)
    AC_CHECK_LIB(lber, ber_init)

    AC_CHECK_HEADERS(ldap.h, ldap_h=["#include <ldap.h>"])
    AC_CHECK_HEADERS(lber.h, lber_h=["#include <lber.h>"])
    AC_CHECK_HEADERS(ldap_ssl.h, ldap_ssl_h=["#include <ldap_ssl.h>"])


  ])

AC_SUBST(ldap_h)
AC_SUBST(lber_h)
AC_SUBST(ldap_ssl_h)
AC_SUBST(apu_has_ldap_netscape_ssl)
AC_SUBST(apu_has_ldap_starttls)
AC_SUBST(apu_has_ldap)

])
