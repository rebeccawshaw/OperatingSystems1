#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/poll.h>



/* name of directory containing relay files */
char *app_dirname = "kleak";
/* base name of per-cpu relay files (e.g. /debug/kleak/cpu0, cpu1, ...) */
char *percpu_basename = "cpu";
/* base name of per-cpu output files (e.g. ./cpu0, cpu1, ...) */
char *percpu_out_basename = "cpu";

/* maximum number of CPUs we can handle - change if more */
#define NR_CPUS 256

/* internal variables */
static size_t subbuf_size = 64;
static size_t n_subbufs = 10;
static unsigned int ncpus;

/* per-cpu internal variables */
static int relay_file[NR_CPUS];
static int out_file[NR_CPUS];
static char *relay_buffer[NR_CPUS];


int main(int argc, char **argv)
{
	
	ncpus = sysconf(_SC_NPROCESSORS_ONLN);

	if (open_app_files())
		return -1;


	printf("Creating channel with %u sub-buffers of size %u.\n", n_subbufs, subbuf_size);
	printf("Logging... Press Control-C to stop.\n");

	close_app_files();
}

//open apps files
static int open_app_files(void)
{
	int i;

	for (i = 0; i < ncpus; i++) {
		open_cpu_files(i, app_dirname, percpu_basename, percpu_out_basename)
			return -1;
		}
	}

	return 0;
}


//Close app files
static void close_app_files(void)
{
	int i;

	for (i = 0; i < ncpus; i++)
		close_cpu_files(i);
}

/**
 *	open_cpu_files - open and mmap buffer and create output file for a cpu
 */
static int open_cpu_files(int cpu, const char *dirname, const char *basename,
			  const char *out_basename)
{
	size_t total_bufsize;
	char tmp[subbuf_size * n_subbufs]; //idk what buff size supposed to be

	//memset(&status[cpu], 0, sizeof(struct buf_status));

	sprintf(tmp, "%s/%s%d", dirname, basename, cpu);
	relay_file[cpu] = open(tmp, O_RDONLY | O_NONBLOCK);
	if (relay_file[cpu] < 0) {
		printf("Couldn't open relay file %s: errcode = %s\n", tmp, strerror(errno));
		return -1;
	}

	sprintf(tmp, "%s%d", out_basename, cpu);
	if((out_file[cpu] = open(tmp, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
		printf("Couldn't open output file %s: errcode = %s\n",tmp, strerror(errno));
		close(relay_file[cpu]);
		return -1;
	}

	total_bufsize = subbuf_size * n_subbufs;
	relay_buffer[cpu] = mmap(NULL, total_bufsize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, relay_file[cpu], 0);
	if(relay_buffer[cpu] == MAP_FAILED)
	{
		printf("Couldn't mmap relay file, total_bufsize (%d) = subbuf_size (%d) * n_subbufs(%d), error = %s \n", total_bufsize, subbuf_size, n_subbufs, strerror(errno));
		close(relay_file[cpu]);
		close(out_file[cpu]);
		return -1;
	}
	
	return 0;
}

/**
*	close_cpu_files - close and munmap buffer and open output file for cpu
*/
static void close_cpu_files(int cpu)
{
	size_t total_bufsize = subbuf_size * n_subbufs;

	munmap(relay_buffer[cpu], total_bufsize);
	close(relay_file[cpu]);
	close(out_file[cpu]);
}
