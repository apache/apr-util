#!/bin/sh

#
# Get a copy of libtool's libtool.m4 and copy it into our aclocal.m4
#
# If we ever have our own .m4 files, then we will want to concatenate
# them altogether into aclocal.m4
#
### we may need to get smarter with these two lines
ltpath=`which libtoolize`
ltpath=`dirname $ltpath`
ltfile=`cd $ltpath/../share/aclocal ; pwd`/libtool.m4
echo "Incorporating $ltfile into aclocal.m4 ..."
cat $ltfile > aclocal.m4

#
# Create the libtool helper files
#
# Note: we always replace the files, and we copy (rather than link) them.
#
echo "Copying libtool helper files ..."
$ltpath/libtoolize --force --copy

#
# Generate the autoconf header (include/apu_config.h) and ./configure
#
echo "Creating include/apu_config.h ..."
autoheader

echo "Creating configure ..."
### do some work to toss config.cache?
autoconf

if [ ! -z $1 ]; then
    echo "Creating list of exported symbols in aprutil.exports ..."
    perl $1/helpers/make_export.pl -o ./aprutil.exports include/*.h
fi
