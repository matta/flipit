#
#  Copyright (C) 1999, 2002, 2003 Matt Armstrong
#  All rights reserved.
# 
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
# 
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
POD2HTML = @POD2HTML@
POD2MAN = @POD2MAN@

AUTOMAKE_OPTIONS = check-news 1.4

bin_PROGRAMS = flipit
flipit_SOURCES = flipit.c cm17a.h cm17a.c conf.c
sysconf_DATA = flipit.conf
man_MANS = flipit.1
noinst_DATA = flipit.html

EXTRA_DIST = cm17a_proto.txt autogen.sh conf.h flipit.pod.in SECURITY \
		solaris.txt $(sysconf_DATA) flipit.1

CUSTOM_DIST = $(man_MANS) $(noinst_DATA)
CUSTOM_VERSION_CHECK = $(srcdir)/README flipit.1 flipit.html

CLEANFILES = *~

# Because we use maintainer mode, we have to tell distcheck to enable
# it when checking the dist.  (this should be the default!)
DISTCHECK_CONFIGURE_FLAGS = --enable-maintainer-mode

DISTCLEANFILES = flipit-*.tar.gz

# Because we use maintainer mode, we have to trick the distcleancheck
# mechanism into not thinking the files in MAINTAINERCLEANFILES are
# problems if they are left after a distclean.  (this should be the
# default!)
MAINTAINERCLEANFILES = flipit.1 flipit.html \
	aclocal.m4 config.h.in configure depcomp missing \
	Makefile.in
distcleancheck_listfiles = \
	find -type f -a ! -name flipit.1 -a ! -name flipit.html

AM_CPPFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\"

if MAINTAINER_MODE
flipit.1: flipit.pod
	$(POD2MAN) --section=1 --release="flipit version @VERSION@" --center="flipit @VERSION@" flipit.pod > flipit.1

flipit.html: flipit.pod
	$(POD2HTML) --noindex flipit.pod > flipit.html
endif

dist-hook: flipit.html
	for file in $(CUSTOM_VERSION_CHECK); do \
		if grep -e "flipit version @VERSION@" $$file > /dev/null ; then :; else \
			echo "ERROR: $$file not updated to version @VERSION@; not releasing" 1>&2; \
			rm -rf $(distdir); \
			exit 1; \
		fi \
	done
	for file in $(CUSTOM_DIST); do \
		d=$(top_builddir); \
		echo cp -p $$d/$$file $(distdir)/$$file; \
		cp -p $$d/$$file $(distdir)/$$file; \
	done
