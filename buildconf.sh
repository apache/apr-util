#!/bin/sh

#
# Build aclocal.m4 from libtool's libtool.m4 and our own M4 files.
#
### we may need to get smarter with these two lines (e.g. PrintPath)
ltpath=`build/PrintPath libtoolize`
ltpath=`dirname $ltpath`
ltfile=`cd $ltpath/../share/aclocal ; pwd`/libtool.m4
echo "Incorporating $ltfile into aclocal.m4 ..."
cat build/apu-conf.m4 $ltfile > aclocal.m4

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
echo "Creating include/private/apu_config.h ..."
autoheader

echo "Creating configure ..."
### do some work to toss config.cache?
autoconf
