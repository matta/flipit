/*-
 * Copyright (C) 1999, 2002, 2003 Matt Armstrong
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "conf.h"
#include "cm17a.h"

static void
usage(char* argv[])
{
	printf(
"Usage: %s [OPTION]... [COMMANDS]...\n"
"\n"
"Control the X10 Firecracker (CM17A).  This program's behavior is\n"
"controlled in part by the %s file, but any\n"
"command line arguments override that file's settings.\n"
"\n"
"OPTIONS:\n"
"\n"
" -h            This help.\n"
" -t tty        Set the tty flipit will use.  This is usually set in the\n"
"               configuration file (%s).\n"
"\n"
"COMMANDS:\n"
"\n"
" flip <house code><device number> [on|off]\n"
" dim <house code><device number> count\n"
" brighten <house code><device number> count\n"
"\n"
"    Flip a device on or off, dim or brighten a device by count\n"
"    steps.\n"
"\n"
"EXAMPLES:\n"
"\n"
"    flipit flip a1 off\n"
"    flipit flip b4 on\n"
"    flipit dim h7 5\n"
"    flipit brighten a2 2\n"
"\n"
"You can pass more than one command at a time.\n"
"\n"
"    flipit flip a1 off flip b4 on dim h7 5\n"
"\n"
"Report bugs to <matt@lickey.com>.  This is flipit version %s\n"
"The flipit home page is http://www.lickey.com/flipit/.\n", 
	argv[0], SYSCONFFILE, SYSCONFFILE, VERSION);
}


static int
parse_args(int argc, char* argv[])
{
	int c;

	while ((c = getopt(argc, argv, "ht:")) != EOF) {
		switch (c) {
		case 'h':	/* help */
			usage(argv);
			exit(0);

		case 't':	/* set the tty */
			conf_set_dev_tty(optarg);
			break;
		}
	}
	return 0;
}

static int
parse_device(const char* device_str, 
	     int* house, int* device)
{
	*house = -1;
	*device = -1;

	if (strlen(device_str) >= 2) {
		*house = toupper(device_str[0]) - 'A';
		*device = atoi(&device_str[1]) - 1;
	}

	if (*house < 0 || *house > 15 || *device < 0 || *device > 15) {

		fprintf(stderr, "Invalid house/device specification %s\n",
			device_str);
		return -1;
	}
	return 0;
}

/*
 * TODO:
 * 2. Removed the need to type "flip" on the command line.
 * 3. Added aliases, so I can say "flipit lava on" to turn on my lava lamp.
 * 4. Added optional logging via syslog.
 * 5. Better debugging code.
 * 6. Configurable timeouts.
 */


int
main(int argc, char* argv[])
{
	int fd;
	int i;
        int house;
        int device;

	if (parse_args(argc, argv) < 0) {
		usage(argv);
		exit(3);
	}

	if (conf_parse() < 0) {
		fprintf(stderr,
                        "Error processing conf file %s, exiting.\n",
                        SYSCONFFILE);
		exit(3);
	}

	if ((fd = open(conf_dev_tty(), O_RDONLY | O_NDELAY)) < 0) {
		fprintf(stderr, "Error opening tty %s\n", conf_dev_tty());
		exit(3);
	}

	/* optind is set by getopt() */
	for (i = optind; i < argc; i++) {
		if (!strcmp(argv[i], "flip") && (i + 2) < argc) {
			enum CM17A_COMMAND cmd;
			if (!strcmp(argv[i + 2], "on")) {
				cmd = CM17A_ON;
			} else if (!strcmp(argv[i + 2], "off")) {
				cmd = CM17A_OFF;
			} else {
				cmd = CM17A_ON;
				fprintf(stderr, "Invalid flip command %s",
					argv[i + 2]);
				usage(argv);
			}
                        if (parse_device(argv[i + 1], &house, &device) == 0) {
                                cm17a_command(fd, house, device, cmd, -1);
                        }
			i += 2;
		} else if (!strcmp(argv[i], "dim") && (i + 2) < argc) {
                        if (parse_device(argv[i + 1], &house, &device) == 0) {
                                cm17a_command(fd, house, device, CM17A_DIM,
                                              atoi(argv[i + 2]));
                        }
                        i += 2;
		} else if (!strcmp(argv[i], "brighten") && (i + 2) < argc) {
                        if (parse_device(argv[i + 1], &house, &device) == 0) {
                                cm17a_command(fd, house, device, CM17A_DIM,
                                              atoi(argv[i + 2]));
                        }
                        i += 2;
		} else {
			fprintf(stderr,
				"Could not recognize trailing args:\n");
			for (; i < argc; i++) {
				fprintf(stderr, " %s\n", argv[i]);
			}
			usage(argv);
		}
	}
	if (i == 1) {
		usage(argv);
	}

	close(fd);
	return 0;
}


/*
  Local Variables:
  mode:c
  c-file-style:"linux"
  tab-width:8
  End:
*/
