#!/bin/sh -

#
# Script that generates all the automake/autoconf generated stuff
# from their sources.
#

req() {
    test -f $1 || get $1
}

req configure.ac
req Makefile.am
req acinclude.m4
req INSTALL
req NEWS
req README
req COPYING
req AUTHORS
req THANKS
req ChangeLog
req flipit.pod.in
req cm17a.c

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
