#!/bin/sh

#
# Generate the autoconf header (include/apu_config.h) and ./configure
#
echo "Creating include/private/apu_config.h ..."
autoheader

echo "Creating configure ..."
### do some work to toss config.cache?
autoconf

#
# If Expat has been bundled, then go and configure the thing
#
if test -d xml/expat; then
  echo "Invoking xml/expat/buildconf.sh ..."
  (cd xml/expat; ./buildconf.sh)
fi
### expat-cvs (from SourceForge's CVS) does not have a buildconf.sh (yet)
