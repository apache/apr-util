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
AC_SUBST(APR_INCLUDES)
])

dnl
dnl APU_CHECK_DB1: is DB1 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN(APU_CHECK_DB1,[
AC_CHECK_HEADER(db1/db.h, [
  AC_CHECK_LIB(db1, dbopen, [
  apu_db_header=db1/db.h
  apu_db_lib=db1
  apu_db_version=1
  ])])])

dnl
dnl APU_CHECK_DB185: is DB1.85 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
dnl NB: BerkelyDB v2 and above can be compiled in 1.85 mode
dnl which has a libdb not libdb1 or libdb185
AC_DEFUN(APU_CHECK_DB185,[
AC_CHECK_HEADER(db_185.h, [
  AC_CHECK_LIB(db, dbopen, [
  apu_db_header=db_185.h
  apu_db_lib=db
  apu_db_version=185
  ])])])

dnl
dnl APU_CHECK_DB2: is DB2 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN(APU_CHECK_DB2,[
AC_CHECK_HEADER(db2/db.h, [
  AC_CHECK_LIB(db2, db_open, [
  apu_db_header=db2/db.h
  apu_db_lib=db2
  apu_db_version=2
  ])])
if test "$apu_db_version" != "2"; then
AC_CHECK_HEADER(db.h, [
  AC_CHECK_LIB(db, db_open, [
  apu_db_header=db.h
  apu_db_lib=db
  apu_db_version=2
  ])])
fi
])

dnl
dnl APU_CHECK_DB3: is DB3 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
AC_DEFUN(APU_CHECK_DB3,[
AC_CHECK_HEADER(db3/db.h, [
  AC_CHECK_LIB(db3, db_create, [
  apu_db_header=db3/db.h
  apu_db_lib=db3
  apu_db_version=3
  ])])
if test "$apu_db_version" != "3"; then
AC_CHECK_HEADER(db.h, [
  AC_CHECK_LIB(db, db_create, [
  apu_db_header=db.h
  apu_db_lib=db
  apu_db_version=3
  ])])
fi
])

dnl
dnl APU_CHECK_DB4: is DB4 present?
dnl
dnl if present: sets apu_db_header, apu_db_lib, and apu_db_version
dnl
dnl At this point in time, DB4 doesn't have some functions that DB3 does.
dnl So look for a function that was removed in DB3 to confirm DB4.  
dnl If it fails, then we know we are DB4 at least.
dnl
AC_DEFUN(APU_CHECK_DB4,[
AC_CHECK_HEADER(db4/db.h, [
  AC_CHECK_LIB(db4, db_create, [
  apu_db_header=db4/db.h
  apu_db_lib=db4
  apu_db_version=4
  ])])
if test "$apu_db_version" != "4"; then
AC_CHECK_HEADER(db.h, [
  AC_CHECK_LIB(db, db_create, [
    AC_CHECK_LIB(db, lock_get, [], [
        apu_db_header=db.h
        apu_db_lib=db
        apu_db_version=4
    ])])])
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

apu_db_header=db.h		# default so apu_select_dbm.h is syntactically correct
apu_db_version=0

AC_ARG_WITH(dbm,
  [  --with-dbm=DBM          choose the DBM type to use.
                          DBM={sdbm,gdbm,db,db1,db185,db2,db3,db4}],[
  if test "$withval" = "yes"; then
    AC_MSG_ERROR([--with-dbm needs to specify a DBM type to use.
One of: sdbm, gdbm, db, db1, db185, db2, db3, db4])
  fi
  requested="$withval"
],[
  requested=default
])
AC_ARG_WITH([gdbm],
[ --with-gdbm=DIR          specify GDBM location], [
    apu_have_gdbm=0
    if test "$withval" = "yes"; then
      AC_CHECK_HEADER(gdbm.h, AC_CHECK_LIB(gdbm, gdbm_open, [apu_have_gdbm=1]))
    elif test "$withval" = "no"; then
      apu_have_gdbm=0
    else
        CPPFLAGS="-I$withval/include"
        LIBS="-L$withval/lib "

        AC_MSG_CHECKING(checking for gdbm in $withval)
        AC_CHECK_HEADER(gdbm.h, AC_CHECK_LIB(gdbm, gdbm_open, [apu_have_gdbm=1]))
        if test "$apu_have_gdbm" != "0"; then
            APR_ADDTO(APRUTIL_EXPORT_LIBS, [-L$withval/lib])
            APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
        fi
    fi
],[
    apu_have_gdbm=0
    AC_CHECK_HEADER(gdbm.h, AC_CHECK_LIB(gdbm, gdbm_open, [apu_have_gdbm=1]))
])
        

dnl We're going to try to find the highest version of Berkeley DB supported.
APU_CHECK_DB4
if test "$apu_db_version" != "4"; then
  APU_CHECK_DB3
  if test "$apu_db_version" != "3"; then
    APU_CHECK_DB2
    if test "$apu_db_version" != "2"; then
      APU_CHECK_DB1
      if test "$apu_db_version" != "1"; then
        APU_CHECK_DB185
      fi
    fi
  fi
fi

dnl Yes, it'd be nice if we could collate the output in an order
dnl so that the AC_MSG_CHECKING would be output before the actual
dnl checks, but it isn't happening now.
AC_MSG_CHECKING(for Berkeley DB)
if test "$apu_db_version" != "0"; then
  apu_have_db=1
  AC_MSG_RESULT(found db$apu_db_version)
else
  AC_MSG_RESULT(not found)
fi

dnl Note that we may have found db3, but the user wants db1.  So, check
dnl explicitly for db1 in this case.  Unfortunately, this means 
dnl repeating the DB tests again.  And, the fact that the APU Berkeley
dnl DB macros can't have the side-effect of setting LIBS.
case "$requested" in
  sdbm)
    apu_use_sdbm=1
    apu_default_dbm=sdbm
    ;;
  gdbm)
    apu_use_gdbm=1
    apu_default_dbm=gdbm
    ;;
  db)
    if test "$apu_db_version" != "0"; then
      apu_use_db=1
      apu_default_dbm=db
    else
      AC_MSG_ERROR(Berkeley db requested, but not found)
    fi
    ;;
  db1)
    APU_CHECK_DB1
    if test "$apu_db_version" = "1"; then
      apu_use_db=1
      apu_default_dbm=db1
    else
      AC_MSG_ERROR(Berkeley db1 not found)
    fi
    ;;
  db185)
    APU_CHECK_DB185
    if test "$apu_db_version" = "185"; then
      apu_use_db=1
      apu_default_dbm=db185
    else
      AC_MSG_ERROR(Berkeley db185 not found)
    fi
    ;;
  db2)
    APU_CHECK_DB2
    if test "$apu_db_version" = "2"; then
      apu_use_db=1
      apu_default_dbm=db2
    else
      AC_MSG_ERROR(Berkeley db2 not found)
    fi
    ;;
  db3)
    APU_CHECK_DB3
    if test "$apu_db_version" = "3"; then
      apu_use_db=1
      apu_default_dbm=db3
    else
      AC_MSG_ERROR(Berkeley db3 not found)
    fi
    ;;
  db4)
    APU_CHECK_DB4
    if test "$apu_db_version" = "4"; then
      apu_use_db=1
      apu_default_dbm=db4
    else
      AC_MSG_ERROR(Berkeley db4 not found)
    fi
    ;;
  default)
    dnl ### use more sophisticated DBMs for the default?
    apu_default_dbm="sdbm (default)"
    apu_use_sdbm=1
    ;;
  *)
    AC_MSG_ERROR([--with-dbm=$look_for is an unknown DBM type.
Use one of: sdbm, gdbm, db, db1, db185, db2, db3, db4])
    ;;
esac

dnl Yes, it'd be nice if we could collate the output in an order
dnl so that the AC_MSG_CHECKING would be output before the actual
dnl checks, but it isn't happening now.
AC_MSG_CHECKING(for default DBM)
AC_MSG_RESULT($apu_default_dbm)

AC_SUBST(apu_use_sdbm)
AC_SUBST(apu_use_gdbm)
AC_SUBST(apu_use_db)

AC_SUBST(apu_have_sdbm)
AC_SUBST(apu_have_gdbm)
AC_SUBST(apu_have_db)
AC_SUBST(apu_db_header)
AC_SUBST(apu_db_version)

dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
dnl we know the library is there.
if test "$apu_have_gdbm" = "1"; then
  APR_ADDTO(APRUTIL_EXPORT_LIBS,[-lgdbm])
  APR_ADDTO(LIBS,[-lgdbm])
fi

if test "$apu_db_version" != "0"; then
  APR_ADDTO(APRUTIL_EXPORT_LIBS,[-l$apu_db_lib])
  APR_ADDTO(LIBS,[-l$apu_db_lib])
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

APR_ADDTO(APRUTIL_INCLUDES, [-I$expat_include_dir])
APR_ADDTO(APRUTIL_EXPORT_LIBS, [$expat_libs])
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
