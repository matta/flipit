#!/bin/sh -

#
# Script that generates all the automake/autoconf generated stuff
# from their sources.
#

echo "Generating aclocal.m4..."
aclocal
echo "Generating config.h.in..."
autoheader
echo "Generating the Makefile.in files..."
automake --add-missing --foreign
echo "Generating the configure script..."
autoconf
echo "Now running configure..."
./configure --enable-maintainer-mode $*
