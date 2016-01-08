/*
 *  exported-global - test exported relay file ops with a global buffer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2005 - Tom Zanussi (zanussi@us.ibm.com), IBM Corp
 *
 * This example creates a global relay file in the debugfs filesystem
 * and uses read(2) to read from it.  It amounts to pretty much the
 * simplest possible usage of relayfs for ad hoc kernel logging.
 * Relayfs itself should be insmod'ed or configured in, but doesn't
 * need to be mounted.
 *
 * Usage:
 *
 * modprobe relayfs
 * mount -t debugfs debugfs /debug
 * insmod ./exported-global-mod.ko
 * ./exported-global [-b subbuf-size -n n_subbufs]
 *
 * captured output will appear on stdout
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

/* name of global relay file */
char *relay_file_name = "/debug/exported-global/global0";

/* internal variables */
static int relay_file;

static void process_data(const char *buf, int len)
{
#if 1
	printf("read %08u bytes:\n", len);
#endif
	/* Write to stdout for now, but we could do so much more... */
	write(1, buf, len);
}

int main(int argc, char **argv)
{
	char buf[4096];
	int rc;

	relay_file = open(relay_file_name, O_RDONLY | O_NONBLOCK);
	if (relay_file < 0) {
		printf("Couldn't open relay file %s: errcode = %s\n",
		       relay_file_name, strerror(errno));
		return -1;
	}

	do {
		rc = read(relay_file, buf, 4096);
		if (rc < 0) {
			perror("read");
			continue;
		} else if (rc > 0) {
			process_data(buf, rc);
			continue;
		}
		usleep(1000);
	} while (1);

	close(relay_file);
}

