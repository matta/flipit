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

/* Macros to form the command data */
#define COMMAND_ON(house, device)	\
((house_codes[house] << 12) | device_codes[device])

#define COMMAND_OFF(house, device)	\
(COMMAND_ON(house, device) | 0x0020)

#define COMMAND_BRIGHT(house)		(bright_codes[house])
#define COMMAND_DIM(house)		\
(COMMAND_BRIGHT(house) | 0x0010)

/* The house code is the high nibble of the command data. */
unsigned char house_codes[16] = {
	0x6,			/* A */
	0x7,			/* B */
	0x4,			/* C */
	0x5,			/* D */

	0x8,			/* E */
	0x9,			/* F */
	0xA,			/* G */
	0xB,			/* H */

	0xE,			/* I */
	0xF,			/* J */
	0xC,			/* K */
	0xD,			/* L */

	0x0,			/* M */
	0x1,			/* N */
	0x2,			/* O */
	0x3,			/* P */
};

/* The remaining 3 nibbles address the specific device within the
   house. Bitwise or in 0x0020 to turn the device off. Notice that
   devices >= 9 are just 0x4000 bitwise ored with the same device
   numbered 8 smaller. */
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

/* To brighten each house, send the following commands.  To dim a
   house, just or in 0x0010 to this value. */
unsigned short bright_codes[16] = {
	0x6088,			/* A */
	0x7088,			/* B */
	0x4088,			/* C */
	0x5088,			/* D */
	0x8088,			/* E */
	0x9088,			/* F */
	0xA088,			/* G */
	0xB088,			/* H */
	0xE088,			/* I */
	0xF088,			/* J */
	0xC088,			/* K */
	0xD088,			/* L */
	0x0088,			/* M */
	0x1088,			/* N */
	0x2088,			/* O */
	0x3088,			/* P */
};


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
reset(int fd)
{
	if (set(fd, RESET) != 0) {
		return -1;
	}
	delay();
	return 0;
}

static int
standby(int fd)
{
	if (set(fd, STANDBY) != 0) {
		return -1;
	}
	delay();
	return 0;
}

static int
header(int fd)
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
command(int fd, int command)
{
	if (reset(fd) || standby(fd)) {
		return -1;
	}

	/* The cm17a seems to need another delay period to settle down
	   after the reset. */
	delay();
	
	if (header(fd) ||
	    write_byte(fd, command >> 8) || 
	    write_byte(fd, command) ||
	    footer(fd)) {
		return -1;
	}

	sleep_us(1000000);

	return 0;
}


void
flip(int fd,
     const char* device_str, 
     const char* operation_str) 
{
	int house = -1;
	int device = -1;

	if (strlen(device_str) >= 2) {
		house = toupper(device_str[0]) - 'A';
		device = atoi(&device_str[1]) - 1;
	}

	if (house < 0 || house > 15 || device < 0 || device > 15) {

		fprintf(stderr, "Invalid house/device specification %s\n",
			device_str);
		return;
	}
	
	if (!strcmp(operation_str, "on")) {
		if (command(fd, COMMAND_ON(house, device)) != 0) {
			fprintf(stderr, "Command failed.\n");
		}
	} else if (!strcmp(operation_str, "off")) {
		if (command(fd, COMMAND_OFF(house, device)) != 0) {
			fprintf(stderr, "Command failed.\n");
		}
	} else {
		fprintf(stderr, "Invalid flip operation %s\n", operation_str);
	}
}


int
main(int argc, char* argv[]) 
{
	int fd;
	int i;

	if (conf_parse() != 0) {
		fprintf(stderr, "Error processing conf file, exiting.\n");
		exit(3);
	}

	if ((fd = open(conf_dev_tty(), O_RDONLY | O_NDELAY)) < 0) {
		fprintf(stderr, "Error opening tty %s\n", conf_dev_tty());
		exit(3);
	}

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "flip") && (i + 2) < argc) {
			flip(fd, argv[i + 1], argv[i + 2]);
			i += 2;
		} else {
			fprintf(stderr, 
				"Could not recognize trailing args:\n");
			for (; i < argc; i++) {
				fprintf(stderr, " %s\n", argv[i]);
			}
		}
	}

	close(fd);
	return 0;
}
