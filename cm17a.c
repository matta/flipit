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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "conf.h"

#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

enum SIGNAL {
	RESET = 0,
	HIGH = TIOCM_RTS,
	LOW = TIOCM_DTR,
	STANDBY = (TIOCM_RTS | TIOCM_DTR)
};

/* The house code is the high nibble of the command data. */
unsigned short house_codes[16] = {
	0x6000,			/* A */
	0x7000,			/* B */
	0x4000,			/* C */
	0x5000,			/* D */
	0x8000,			/* E */
	0x9000,			/* F */
	0xA000,			/* G */
	0xB000,			/* H */
	0xE000,			/* I */
	0xF000,			/* J */
	0xC000,			/* K */
	0xD000,			/* L */
	0x0000,			/* M */
	0x1000,			/* N */
	0x2000,			/* O */
	0x3000,			/* P */
};

/* The device codes are described by bits 3, 4, 6, and 10. */
#define DEVICE_BITS 0x0458
unsigned short device_codes[16] = {
	0x0000,			/* 1 */
	0x0010,			/* 2 */
	0x0008,			/* 3 */
	0x0018,			/* 4 */
	0x0040,			/* 5 */
	0x0050,			/* 6 */
	0x0048,			/* 7 */
	0x0058,			/* 8 */
	0x0400,			/* 9 */
	0x0410,			/* 10 */
	0x0408,			/* 11 */
	0x0418,			/* 12 */
	0x0440,			/* 13 */
	0x0450,			/* 14 */
	0x0448,			/* 15 */
	0x0458,			/* 16 */
};

/* The command codes are described by bits 1, 2, 5, 7, 8, 9, and
   11. */
unsigned short command_codes[] = {
	0x0000,			/* ON */
	0x0020,			/* OFF */
	0x0088,			/* BRIGHT */
	0x0098,			/* DIM */
};

enum command_index {
	ON, OFF, BRIGHTEN, DIM
};

#define ELEMENT_COUNT(a) (sizeof(a) / sizeof((a)[0]))

unsigned short
cm17a_command_word(int house, 
		   int device, 
		   enum command_index command)
{
	unsigned short retval;

	assert(house >= 0 && house < ELEMENT_COUNT(house_codes));
	assert(device >= 0 && device < ELEMENT_COUNT(device_codes));
	assert(command >= 0 && command < ELEMENT_COUNT(command_codes));
	
	switch (command) {
	case BRIGHTEN:
	case DIM:
		retval = house_codes[house] | command_codes[command];
		break;

	default:
		retval = (house_codes[house] | 
			  device_codes[device] |
			  command_codes[command]);
		break;
	}

	return retval;
}

static void
sleep_us(unsigned int nusecs)
{
	struct timeval tval;

	tval.tv_sec = nusecs / 1000000;
	tval.tv_usec = nusecs % 1000000;
	select(0, NULL, NULL, NULL, &tval);
}

static void
delay(void)
{
	sleep_us(500);
}

static int
set(int fd, enum SIGNAL signal)
{
	int lines = signal;
	int current;
	/* FIXME: we should just remember this in a static
	   variable. */
	if (ioctl(fd, TIOCMGET, &current) != 0) {
		return -1;
	}
	current &= ~RESET;
	current |= signal;
	return ioctl(fd, TIOCMSET, &lines);
}

static int
write_byte(int fd, unsigned char byte)
{
	int mask = 0x80;
	do {
		enum SIGNAL signal = (byte & mask) ? HIGH : LOW;
		if (set(fd, signal) != 0) {
			return -1;
		}
		delay();
		if (set(fd, STANDBY) != 0) {
			return -1;
		}
		delay();
		mask >>= 1;
	} while (mask);
	return 0;
}

int
write_stream(int fd, 
	     const unsigned char* data, 
	     size_t byte_count) 
{
	int i;
	for (i = 0; i < byte_count; i++) {
		int ret = write_byte(fd, data[i]);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

static int
cm17a_standby(int fd)
{
	if (set(fd, STANDBY) != 0) {
		return -1;
	}
	delay();
	return 0;
}

static int
cm17a_header(int fd)
{
	/* Header bits taken straight from the CM17A spec. */
	const unsigned char header[] = {
		0xD5, 0xAA
	};
	return write_stream(fd, header, sizeof(header));
}

static int
footer(int fd)
{
	/* Footer bits taken straight from the CM17A spec. */
	return write_byte(fd, 0xAD);
}

static int
cm17a_write_command(int fd, 
		    int house, 
		    int device, 
		    enum command_index command)
{
	short command_word;

	command_word = cm17a_command_word(house, device, command);

	if (cm17a_standby(fd)) {
		return -1;
	}

	/* The cm17a seems to need another delay period after a
	   standby to settle down after the reset. */
	delay();

	if (cm17a_header(fd) ||
	    write_byte(fd, command_word >> 8) || 
	    write_byte(fd, command_word) ||
	    footer(fd)) {
		return -1;
	}

	sleep_us(500000);

	return 0;
}

int
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

void
flip(int fd,
     const char* device_str, 
     const char* operation_str) 
{
	int house;
	int device;

	if (parse_device(device_str, &house, &device)) {
		return;
	}

	if (!strcmp(operation_str, "on")) {
		if (cm17a_write_command(fd, house, device, ON) != 0) {
			fprintf(stderr, "Command failed.\n");
		}
	} else if (!strcmp(operation_str, "off")) {
		if (cm17a_write_command(fd, house, device, OFF) != 0) {
			fprintf(stderr, "Command failed.\n");
		}
	} else {
		fprintf(stderr, "Invalid flip operation %s\n", operation_str);
	}
}

void
dim_or_brighten(int fd, 
		const char* device_str, 
		const char* steps_str,
		enum command_index cmd)
{
	int house;
	int device;
	int steps;

	if (parse_device(device_str, &house, &device)) {
		return;
	}

	steps = atoi(steps_str);

	if (steps < 1 || steps > 6) {
		fprintf(stderr, 
			"Invalid steps (%d).  Must be [1..6].\n", 
			steps);
		return;
	}

	/* Turn the device we wish to control on first.  If we don't
	   do this, the dim command gets handled by the last device
	   that got turned on.  The cm17a dim/brighten command doesn't
	   use a device code. */
	   
	if (cm17a_write_command(fd, house, device, ON) != 0) {
		fprintf(stderr, "Command failed.\n");
	}

	do {
		if (cm17a_write_command(fd, house, device, cmd)) {
			fprintf(stderr, "Command failed.\n");
		}
	} while (--steps > 0);
}

void
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



int
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

int
main(int argc, char* argv[]) 
{
	int fd;
	int i;

	if (parse_args(argc, argv) < 0) {
		usage(argv);
		exit(3);
	}

	if (conf_parse() < 0) {
		fprintf(stderr, "Error processing conf file, exiting.\n");
		exit(3);
	}

	if ((fd = open(conf_dev_tty(), O_RDONLY | O_NDELAY)) < 0) {
		fprintf(stderr, "Error opening tty %s\n", conf_dev_tty());
		exit(3);
	}

	/* optind is set by getopt() */
	for (i = optind; i < argc; i++) {
		if (!strcmp(argv[i], "flip") && (i + 2) < argc) {
			flip(fd, argv[i + 1], argv[i + 2]);
			i += 2;
		} else if (!strcmp(argv[i], "dim") && (i + 2) < argc) {
			dim_or_brighten(fd, argv[i + 1], argv[i + 2], 
					DIM);
			i += 2;
		} else if (!strcmp(argv[i], "brighten") && (i + 2) < argc) {
			dim_or_brighten(fd, argv[i + 1], argv[i + 2], 
					BRIGHTEN);
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
