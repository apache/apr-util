dnl
dnl custom autoconf rules for APRUTIL
dnl

dnl
dnl APU_FIND_APR: figure out where APR is located
dnl
AC_DEFUN(APU_FIND_APR,[

  dnl use the find_apr.m4 script to locate APR. sets apr_found and apr_config
  APR_FIND_APR
  if test "$apr_found" = "no"; then
    AC_MSG_ERROR(APR could not be located. Please use the --with-apr option.)
  fi

  APR_BUILD_DIR="`$apr_config --installbuilddir`"

  dnl make APR_BUILD_DIR an absolute directory (we'll need it in the
  dnl sub-projects in some cases)
  APR_BUILD_DIR="`cd $APR_BUILD_DIR && pwd`"

  APR_INCLUDES="`$apr_config --includes`"
  APR_LIBS="`$apr_config --link-libtool --libs`"
  APR_SO_EXT="`$apr_config --apr-so-ext`"
  APR_LIB_TARGET="`$apr_config --apr-lib-target`"

  AC_SUBST(APR_INCLUDES)
  AC_SUBST(APR_LIBS)

  dnl ### would be nice to obsolete these
  APR_SOURCE_DIR="`$apr_config --srcdir`"
  AC_SUBST(APR_BUILD_DIR)
  AC_SUBST(APR_SOURCE_DIR)
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
  ])])
if test "$apu_db_version" != "1"; then
AC_CHECK_HEADER(db.h, [
  AC_CHECK_LIB(c, dbopen, [
  apu_db_header=db.h
  apu_db_lib=
  apu_db_version=1
  ], [
  AC_CHECK_LIB(db1, dbopen, [
  apu_db_header=db.h
  apu_db_lib=db1
  apu_db_version=1
  ])])])
fi
])

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
  ], [
  AC_CHECK_LIB(db-4.0, db_create, [
  apu_db_header=db4/db.h
  apu_db_lib=db-4.0
  apu_db_version=4
  ])])])
if test "$apu_db_version" != "4"; then
AC_CHECK_HEADER(db.h, [
  AC_CHECK_LIB(db, db_create, [
    AC_CHECK_LIB(db, lock_get, [], [
        apu_db_header=db.h
        apu_db_lib=db
        apu_db_version=4
    ])], [
      AC_CHECK_LIB(db, db_create_4000, [
      apu_db_header=db.h
      apu_db_lib=db
      apu_db_version=4
    ], [
      AC_CHECK_LIB(db, db_create_4001, [
      apu_db_header=db.h
      apu_db_lib=db
      apu_db_version=4
    ])])])])
fi
])

AC_DEFUN(APU_CHECK_BERKELEY_DB,[
if test "$apu_want_db" != "0"; then
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
fi
AC_MSG_CHECKING(for Berkeley DB)
if test "$apu_db_version" != "0"; then
  apu_have_db=1
  AC_MSG_RESULT(found db$apu_db_version)
else
  AC_MSG_RESULT(not found)
fi


])
dnl
dnl APU_CHECK_DBM: see what kind of DBM backend to use for apr_dbm.
dnl
AC_DEFUN(APU_CHECK_DBM,[
apu_use_sdbm=0
apu_use_ndbm=0
apu_use_gdbm=0
apu_use_db=0
dnl it's in our codebase
apu_have_sdbm=1
apu_have_gdbm=0
apu_have_ndbm=0
apu_have_db=0

apu_db_header=db.h		# default so apu_select_dbm.h is syntactically correct
apu_db_version=0

if test -n "$apu_db_xtra_libs"; then
  saveddbxtralibs="$LIBS"
  LIBS="$apu_db_xtra_libs $LIBS"
fi

AC_ARG_WITH(dbm,
  [  --with-dbm=DBM          choose the DBM type to use.
                          DBM={sdbm,gdbm,ndbm,db,db1,db185,db2,db3,db4}],[
  if test "$withval" = "yes"; then
    AC_MSG_ERROR([--with-dbm needs to specify a DBM type to use.
One of: sdbm, gdbm, ndbm, db, db1, db185, db2, db3, db4])
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
            APR_ADDTO(APRUTIL_LDFLAGS, [-L$withval/lib])
            APR_ADDTO(APRUTIL_INCLUDES, [-I$withval/include])
        fi
    fi
],[
    apu_have_gdbm=0
    AC_CHECK_HEADER(gdbm.h, AC_CHECK_LIB(gdbm, gdbm_open, [apu_have_gdbm=1]))
])

AC_ARG_WITH([ndbm],
[ --with-ndbm=PATH 
    Find the NDBM header and library in \`PATH/include' and 
    \`PATH/lib'.  If PATH is of the form \`HEADER:LIB', then search 
    for header files in HEADER, and the library in LIB.  If you omit
    the \`=PATH' part completely, the configure script will search
    for NDBM in a number of standard places.], [

    apu_have_ndbm=0
    if test "$withval" = "yes"; then
      AC_MSG_CHECKING(checking for ndbm in the usual places)
      apu_want_ndbm=1
      NDBM_INC=""
      NDBM_LDFLAGS=""
    elif test "$withval" = "no"; then
      apu_want_ndbm=0
    else
      apu_want_ndbm=1
      case "$withval" in
        *":"*)
          NDBM_INC="-I`echo $withval |sed -e 's/:.*$//'`"
          NDBM_LDFLAGS="-L`echo $withval |sed -e 's/^.*://'`"
          AC_MSG_CHECKING(checking for ndbm includes with $NDBM_INC libs with $NDBM_LDFLAGS )
        ;;
        *)
          NDBM_INC="-I$withval/include"
          NDBM_LDFLAGS="-L$withval/lib"
          AC_MSG_CHECKING(checking for ndbm includes in $withval)
        ;;
      esac
    fi

    save_cppflags="$CPPFLAGS"
    save_ldflags="$LDFLAGS"
    CPPFLAGS="$CPPFLAGS $NDBM_INC"
    LDFLAGS="$LDFLAGS $NDBM_LDFLAGS"
    dnl db_ndbm_open is what sleepcat's compatibility library actually has in it's lib
    if test "$apu_want_ndbm" != "0"; then
      AC_CHECK_HEADER(ndbm.h, 
        AC_CHECK_LIB(c, dbm_open, [apu_have_ndbm=1;apu_ndbm_lib=c],
            AC_CHECK_LIB(dbm, dbm_open, [apu_have_ndbm=1;apu_ndbm_lib=dbm],
                AC_CHECK_LIB(db, dbm_open, [apu_have_ndbm=1;apu_ndbm_lib=db],
                    AC_CHECK_LIB(db, __db_ndbm_open, [apu_have_ndbm=1;apu_ndbm_lib=db])
                )
            )
        )
      )
      if test "$apu_have_ndbm" != "0";  then
            if test "$withval" != "yes"; then
                APR_ADDTO(APRUTIL_INCLUDES, [$NDBM_INC])
                APR_ADDTO(APRUTIL_LDFLAGS, [$NDBM_LDFLAGS])
            fi
      elif test "$withval" != "yes"; then
            AC_ERROR( NDBM not found in the specified directory)
      fi
    fi
    CPPFLAGS="$save_cppflags"
    LDFLAGS="$save_ldflags"
],[
    dnl don't check it no one has asked us for it
    apu_have_ndbm=0
])


dnl We're going to try to find the highest version of Berkeley DB supported.
AC_ARG_WITH([berkeley-db],
[--with-berkeley-db=PATH
     Find the Berkeley DB header and library in \`PATH/include' and
    \`PATH/lib'.  If PATH is of the form \`HEADER:LIB', then search
    for header files in HEADER, and the library in LIB.  If you omit
    the \`=PATH' part completely, the configure script will search
    for Berkeley DB in a number of standard places.
], [
   if test "$withval" = "yes"; then
      apu_want_db=1
      BDB_INC=""
      BDB_LDFLAGS=""
    elif test "$withval" = "no"; then
      apu_want_db=0
    else
      apu_want_db=1
      case "$withval" in 
        *":"*)
          BDB_INC="-I`echo $withval |sed -e 's/:.*$//'`"
          BDB_LDFLAGS="-L`echo $withval |sed -e 's/^.*://'`"
        ;;
        *)
          BDB_INC="-I$withval/include"
          BDB_LDFLAGS="-L$withval/lib"
        ;;
      esac
      AC_MSG_RESULT(looking for berkeley-db includes with $BDB_INC)
      AC_MSG_RESULT(looking for berkeley-db libs with $BDB_LDFLAGS)
    fi
    save_cppflags="$CPPFLAGS"
    save_ldflags="$LDFLAGS"
    CPPFLAGS="$CPPFLAGS $BDB_INC"
    LDFLAGS="$LDFLAGS $BDB_LDFLAGS"
    if test "$apu_want_db" != "0"; then
        APU_CHECK_BERKELEY_DB
        if test "$apu_db_version" != "0"; then
            if test "$withval" != "yes"; then
                APR_ADDTO(APRUTIL_INCLUDES, [$BDB_INC])
                APR_ADDTO(APRUTIL_LDFLAGS, [$BDB_LDFLAGS])
            fi
        elif test "$withval" != "yes"; then
            AC_ERROR( Berkeley DB not found in the specified directory)
        fi
    fi 
    CPPFLAGS="$save_cppflags"
    LDFLAGS="$save_ldflags"
],[
    APU_CHECK_BERKELEY_DB
])


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
  ndbm)
    apu_use_ndbm=1
    apu_default_dbm=ndbm
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
Use one of: sdbm, gdbm, ndbm, db, db1, db185, db2, db3, db4])
    ;;
esac

if test -n "$apu_db_xtra_libs"; then
  LIBS="$saveddbxtralibs"
fi

dnl Yes, it'd be nice if we could collate the output in an order
dnl so that the AC_MSG_CHECKING would be output before the actual
dnl checks, but it isn't happening now.
AC_MSG_CHECKING(for default DBM)
AC_MSG_RESULT($apu_default_dbm)

AC_SUBST(apu_use_sdbm)
AC_SUBST(apu_use_gdbm)
AC_SUBST(apu_use_ndbm)
AC_SUBST(apu_use_db)

AC_SUBST(apu_have_sdbm)
AC_SUBST(apu_have_gdbm)
AC_SUBST(apu_have_ndbm)
AC_SUBST(apu_have_db)
AC_SUBST(apu_db_header)
AC_SUBST(apu_db_version)

dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
dnl we know the library is there.
if test "$apu_have_gdbm" = "1"; then
  APR_ADDTO(APRUTIL_EXPORT_LIBS,[-lgdbm])
  APR_ADDTO(APRUTIL_LIBS,[-lgdbm])
fi

if test "$apu_have_ndbm" = "1"; then
  APR_ADDTO(APRUTIL_EXPORT_LIBS,[-l$apu_ndbm_lib])
  APR_ADDTO(APRUTIL_LIBS,[-l$apu_ndbm_lib])
fi


if test "$apu_db_version" != "0"; then
  if test -n "$apu_db_lib"; then
    APR_ADDTO(APRUTIL_EXPORT_LIBS,[-l$apu_db_lib])
    APR_ADDTO(APRUTIL_LIBS,[-l$apu_db_lib])
    if test -n "apu_db_xtra_libs"; then
      APR_ADDTO(APRUTIL_EXPORT_LIBS,[$apu_db_xtra_libs])
      APR_ADDTO(APRUTIL_LIBS,[$apu_db_xtra_libs])
    fi
  fi
fi

])

dnl
dnl APU_TEST_EXPAT(directory): test if Expat is located in the specified dir
dnl
dnl if present: sets expat_include_dir, expat_libs, possibly expat_old
dnl
AC_DEFUN(APU_TEST_EXPAT,[
  AC_MSG_CHECKING(for Expat in ifelse($2,,$1,$2))

  expat_libtool=""

  if test -r "$1/lib/expat.h.in"; then
    dnl Expat 1.95.* distribution
    expat_include_dir="$1/lib"
    expat_ldflags="-L$1/lib"
    expat_libs="-lexpat"
    expat_libtool="$1/lib/libexpat.la"
  elif test -r "$1/include/expat.h" -a \
    -r "$1/lib/libexpat.la"; then
    dnl Expat 1.95.* installation (with libtool)
    expat_include_dir="$1/include"
    expat_ldflags="-L$1/lib"
    expat_libs="-lexpat"
    expat_libtool="$1/lib/libexpat.la"
  elif test -r "$1/include/expat.h" -a \
    -r "$1/lib64/libexpat.la"; then
    dnl Expat 1.95.* installation on certain 64-bit platforms (with libtool)
    expat_include_dir="$1/include"
    expat_ldflags="-L$1/lib64"
    expat_libs="-lexpat"
    expat_libtool="$1/lib64/libexpat.la"
  elif test -r "$1/include/expat.h" -a \
    -r "$1/lib/libexpat.a"; then
    dnl Expat 1.95.* installation (without libtool)
    dnl FreeBSD textproc/expat2
    expat_include_dir="$1/include"
    expat_ldflags="-L$1/lib"
    expat_libs="-lexpat"
  elif test -r "$1/xmlparse.h"; then
    dnl maybe an expat-lite. use this dir for both includes and libs
    expat_include_dir="$1"
    expat_ldflags="-L$1"
    expat_libs="-lexpat"
    expat_libtool="$1/libexpat.la"
    expat_old=yes
  elif test -r "$1/include/xmlparse.h" -a \
       -r "$1/lib/libexpat.a"; then
    dnl previously installed expat
    expat_include_dir="$1/include"
    expat_ldflags="-L$1/lib"
    expat_libs="-lexpat"
    expat_old=yes
  elif test -r "$1/include/xml/xmlparse.h" -a \
       -r "$1/lib/xml/libexpat.a"; then
    dnl previously installed expat
    expat_include_dir="$1/include/xml"
    expat_ldflags="-L$1/lib"
    expat_libs="-lexpat"
    expat_old=yes
  elif test -r "$1/include/xmltok/xmlparse.h"; then
    dnl Debian distribution
    expat_include_dir="$1/include/xmltok"
    expat_ldflags="-L$1/lib"
    expat_libs="-lxmlparse -lxmltok"
    expat_old=yes
  elif test -r "$1/include/xml/xmlparse.h" -a \
       -r "$1/lib/libexpat.a"; then
    dnl FreeBSD textproc/expat package
    expat_include_dir="$1/include/xml"
    expat_ldflags="-L$1/lib"
    expat_libs="-lexpat"
    expat_old=yes
  elif test -r "$1/xmlparse/xmlparse.h"; then
    dnl Expat 1.0 or 1.1 source directory
    expat_include_dir="$1/xmlparse"
    expat_ldflags="-L$1"
    expat_libs="-lexpat"
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
      dnl For /usr installs of expat, we can't specify -L/usr/lib
      if test "$d" = "/usr"; then
        expat_ldflags=""
      fi
      break
    fi
  done
fi
if test -z "$expat_include_dir"; then
  AC_MSG_ERROR([could not locate Expat. use --with-expat])
fi

dnl If this expat doesn't use libtool natively, we'll mimic it for our
dnl dependency library generation.
if test -z "$expat_libtool"; then
  expat_libtool="$expat_ldflags $expat_libs" 
fi

if test -n "$expat_old"; then
  AC_DEFINE(APR_HAVE_OLD_EXPAT, 1, [define if Expat 1.0 or 1.1 was found])
fi

dnl special-case the bundled distribution (use absolute dirs)
if test "$expat_include_dir" = "xml/expat/lib" -o "$expat_include_dir" = "xml/expat-cvs/lib"; then
  bundled_subdir="`echo $expat_include_dir | sed -e 's%/lib%%'`"
  APR_SUBDIR_CONFIG($bundled_subdir, [--prefix=$prefix --exec-prefix=$exec_prefix --libdir=$libdir --includedir=$includedir --bindir=$bindir])
  expat_include_dir=$top_builddir/$bundled_subdir/lib
  expat_ldflags="-L$top_builddir/$bundled_subdir/lib"
  expat_libs="-lexpat"
  expat_libtool=$top_builddir/$bundled_subdir/lib/libexpat.la
  APR_XML_SUBDIRS="`echo $bundled_subdir | sed -e 's%xml/%%'`"
  APR_ADDTO(APRUTIL_EXPORT_LIBS, [$expat_libtool])
else
if test "$expat_include_dir" = "$abs_srcdir/xml/expat/include" -o "$expat_include_dir" = "$abs_srcdir/xml/expat/lib"; then
  dnl This is a bit of a hack.  This only works because we know that
  dnl we are working with the bundled version of the software.
  bundled_subdir="xml/expat"
  APR_SUBDIR_CONFIG($bundled_subdir, [--prefix=$prefix --exec-prefix=$exec_prefix --libdir=$libdir --includedir=$includedir --bindir=$bindir])
  expat_include_dir=$top_builddir/$bundled_subdir/lib
  expat_ldflags="-L$top_builddir/$bundled_subdir/lib"
  expat_libs="-lexpat"
  expat_libtool=$top_builddir/$bundled_subdir/lib/libexpat.la
  APR_XML_SUBDIRS="`echo $bundled_subdir | sed -e 's%xml/%%'`"
  APR_ADDTO(APRUTIL_EXPORT_LIBS, [$expat_libtool])
else
  APR_ADDTO(APRUTIL_EXPORT_LIBS, [$expat_libs])
fi
fi
APR_XML_DIR=$bundled_subdir
APR_XML_EXPAT_OLD=$expat_old
AC_SUBST(APR_XML_SUBDIRS)
AC_SUBST(APR_XML_DIR)
AC_SUBST(APR_XML_EXPAT_OLD)

if test "$expat_include_dir" != "/usr/include"; then
  APR_ADDTO(APRUTIL_INCLUDES, [-I$expat_include_dir])
fi
APR_ADDTO(APRUTIL_LDFLAGS, [$expat_ldflags])
APR_ADDTO(APRUTIL_LIBS, [$expat_libtool])
])


dnl 
dnl Find a particular LDAP library
dnl
AC_DEFUN(APU_FIND_LDAPLIB,[
  if test ${apu_has_ldap} != "1"; then
    ldaplib=$1
    extralib=$2
    unset ac_cv_lib_${ldaplib}_ldap_init
    unset ac_cv_lib_${ldaplib}___ldap_init
    AC_CHECK_LIB(${ldaplib}, ldap_init, 
      [
        APR_ADDTO(APRUTIL_EXPORT_LIBS,[-l${ldaplib} ${extralib}])
        APR_ADDTO(APRUTIL_LIBS,[-l${ldaplib} ${extralib}])
        AC_CHECK_LIB(${ldaplib}, ldapssl_install_routines, apu_has_ldap_netscape_ssl="1", , ${extralib})
        AC_CHECK_LIB(${ldaplib}, ldap_start_tls_s, apu_has_ldap_starttls="1", , ${extralib})
        apu_has_ldap="1";
      ], , ${extralib})
  fi
])


dnl
dnl APU_FIND_LDAP: figure out where LDAP is located
dnl
AC_DEFUN(APU_FIND_LDAP,[

echo $ac_n "${nl}checking for ldap support..."

apu_has_ldap="0";
apu_has_ldap_netscape_ssl="0"
apu_has_ldap_starttls="0"

AC_ARG_WITH(ldap-include,[  --with-ldap-include=path  path to ldap include files with trailing slash])
AC_ARG_WITH(ldap-lib,[  --with-ldap-lib=path    path to ldap lib file])
AC_ARG_WITH(ldap,[  --with-ldap=library     ldap library to use],
  [
    save_cppflags="$CPPFLAGS"
    save_ldflags="$LDFLAGS"
    save_libs="$LIBS"
    if test -n "$with_ldap_include"; then
      CPPFLAGS="$CPPFLAGS -I$with_ldap_include"
      APR_ADDTO(APRUTIL_INCLUDES, [-I$with_ldap_include])
    fi
    if test -n "$with_ldap_lib"; then
      LDFLAGS="$LDFLAGS -L$with_ldap_lib"
      APR_ADDTO(APRUTIL_LDFLAGS, [-L$with_ldap_lib])
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
      APU_FIND_LDAPLIB("ldap", "-llber -lresolv")
      APU_FIND_LDAPLIB("ldap", "-llber -lsocket -lnsl -lresolv")
      APU_FIND_LDAPLIB("ldap", "-ldl -lpthread")
    else
      APU_FIND_LDAPLIB($LIBLDAP)
      APU_FIND_LDAPLIB($LIBLDAP, "-lresolv")
      APU_FIND_LDAPLIB($LIBLDAP, "-lsocket -lnsl -lresolv")
      APU_FIND_LDAPLIB($LIBLDAP, "-ldl -lpthread")
    fi

    test ${apu_has_ldap} != "1" && AC_MSG_ERROR(could not find an LDAP library)
    AC_CHECK_LIB(lber, ber_init)

    AC_CHECK_HEADERS(ldap.h, ldap_h=["#include <ldap.h>"])
    AC_CHECK_HEADERS(lber.h, lber_h=["#include <lber.h>"])
    AC_CHECK_HEADERS(ldap_ssl.h, ldap_ssl_h=["#include <ldap_ssl.h>"])

    CPPFLAGS=$save_cppflags
    LDFLAGS=$save_ldflags
    LIBS=$save_libs
  ])

AC_SUBST(ldap_h)
AC_SUBST(lber_h)
AC_SUBST(ldap_ssl_h)
AC_SUBST(apu_has_ldap_netscape_ssl)
AC_SUBST(apu_has_ldap_starttls)
AC_SUBST(apu_has_ldap)

])
