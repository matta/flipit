/*-
 * Copyright (c) 1999 Matt Armstrong
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "conf.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define SYSCONFFILE SYSCONFDIR ## "/flipit.conf"

static char* dev_tty = 0;

/* Return the tty device we should open. */
const char* 
conf_dev_tty(void)
{
	return dev_tty;
}

static char* 
eatws(char* p)
{
	while (*p && isspace(*p)) {
		p++;
	}
	return p;
}

int conf_parse(void)
{
	FILE* conf;
	char buffer[4097];
	int line;

	conf = fopen(SYSCONFFILE, "r");
	if (!conf) {
		fprintf(stderr, "Could not open %s\n", SYSCONFFILE);
		return -1;
	}

	line = 0;
	while (fgets(buffer, sizeof(buffer), conf) != 0) {
		int length;
		char* variable;
		char* setting;
		char* p;

		++line;

		/* Make sure we read a whole line and then strip
                   trailing newline. */
		length = strlen(buffer);
		if (buffer[length - 1] != '\n') {
			fprintf(stderr, "%s line %d: line too long.\n",
				SYSCONFFILE, line);
			return -1;
		}
		buffer[length - 1] = '\0';

		p = eatws(buffer);
		if (*p == '\0' || *p == '#') {
			continue;
		}

		if ((variable = strtok(p, " \t")) == NULL) {
			fprintf(stderr, "%s line %d: parse error.\n",
				SYSCONFFILE, line);
			return -1;
		}
		if ((setting = strtok(NULL, " \t")) == NULL) {
			fprintf(stderr, "%s line %d: parse error.\n",
				SYSCONFFILE, line);
			return -1;
		}

		if (!strcmp(variable, "tty")) {
			dev_tty = (char*)malloc(strlen(setting) + 1);
			if (!dev_tty) {
				return -1;
			}
			strcpy(dev_tty, setting);
		} else {
			fprintf(stderr, "%s line %d: invalid option %s\n",
				SYSCONFFILE, line, variable);
		}
	}

	fclose(conf);
	return 0;
}
