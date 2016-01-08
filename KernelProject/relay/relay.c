#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

/* name of global relay file */
char *relay_file_name = "/sys/kernel/debug/kleak/kleak0";

/* internal variables */
static int relay_file;

static void process_data(const char *buf, int len)
{
#if 1
	printf("read %08u bytes:\n", len);
#endif
	/* Write to stdout FOR NOW */
	write(1, buf, len);
}

int main(int argc, char **argv)
{
	char buf[64];
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

