dnl
dnl Copyright (c) 1999-2000, 2003 Matt Armstrong
dnl All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
dnl ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
dnl IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
dnl FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
dnl OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
dnl HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
dnl LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
dnl OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
dnl SUCH DAMAGE.
dnl
AC_INIT(flipit, 0.3.4, matt@lickey.com)
AC_CONFIG_SRCDIR([cm17a.c])
AC_PREREQ(2.53)
AM_INIT_AUTOMAKE

dnl if test x"$missing_dir" = x; then
dnl 	echo "Whoopse, AM_INIT_AUTOMAKE used to set $missing_dir"
dnl 	missing_dir=`cd $ac_aux_dir && pwd`
dnl fi
FLIPIT_MISSING_POD_PROG(POD2HTML, pod2html, $missing_dir)
FLIPIT_MISSING_POD_PROG(POD2MAN, pod2man, $missing_dir)

AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_PROG_CC

AC_STDC_HEADERS
AC_HAVE_HEADERS(fcntl.h sys/ioctl.h sys/time.h sys/types.h unistd.h termio.h)

AC_OUTPUT([Makefile flipit.pod])
