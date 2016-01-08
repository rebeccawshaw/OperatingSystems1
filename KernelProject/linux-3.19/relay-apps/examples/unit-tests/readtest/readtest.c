/*
 * readtest - user program used to test read(2)
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
 * Usage:
 *
 * mount -t debugfs debugfs /debug
 * insmod readtest.ko
 * ./readtest
 *
 * captured output will appear in ./cpu0...cpuN-1
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>

/* name of directory containing relay files */
char *app_dirname = "/debug/readtest";
/* base name of per-cpu relay files (e.g. /debug/readtest/cpu0, cpu1, ...) */
char *percpu_basename = "cpu";
/* base name of per-cpu output files (e.g. ./cpu0, cpu1, ...) */
char *percpu_out_basename = "cpu";

/* maximum number of CPUs we can handle - change if more */
#define NR_CPUS 256

/* internal variables */
static size_t subbuf_size = 262144;
static size_t n_subbufs = 4;
static unsigned int ncpus;

/* per-cpu internal variables */
static int relay_file[NR_CPUS];
static int out_file[NR_CPUS];
static char *relay_buffer[NR_CPUS];
static pthread_t reader[NR_CPUS];

/* control files */
static int produced_file[NR_CPUS];
static int consumed_file[NR_CPUS];

/* per-cpu buffer info */
static struct buf_status
{
	size_t produced;
	size_t consumed;	
	size_t max_backlog; /* max # sub-buffers ready at one time */
} status[NR_CPUS];

/* readtest-specific definitions */
#define DEFAULT_WRITE_N (10)
#define DEFAULT_WRITE_SIZE (32)

static size_t write_n = DEFAULT_WRITE_N;
static size_t write_size = DEFAULT_WRITE_SIZE;
static int overwrite_mode;
static unsigned write_iter = 1;
static unsigned expect_result;
static unsigned random_n;
static unsigned random_l;
static unsigned ignore_counts;
static size_t kthread_secs;

static int logging;
static int got_totals;
static int done_writing;
static int test_done;

static unsigned long long read_bytes;
//static unsigned long long wrote_bytes;
static unsigned wrote_n;

static unsigned kernel_wrote;
static unsigned long long kernel_wrote_bytes;
static unsigned kernel_lost;

static unsigned prev_seq = -1;
static unsigned read_n;
static unsigned missing;

static size_t control_read(const char *dirname,
			   const char *filename);
static size_t control_read_ll(const char *dirname,
			      const char *filename);
static void control_write(const char *dirname,
			  const char *filename,
			  size_t val);
static int open_app_files(void);
static void close_app_files(void);
static int summarize(void);
static int kill_percpu_threads(int n);
static int create_percpu_threads(void);

static void usage(void)
{
	fprintf(stderr, "readtest [-b subbuf_size -n n_subbufs and a bunch of other options]\n");
	exit(1);
}

static void sigproc(int signum)
{
	int rc;
	
	if (!logging)
		return;

	logging = 0;

	control_write(app_dirname, "enabled", 0);
	sleep(2);
	kill_percpu_threads(ncpus);
	rc = summarize();
	close_app_files();
	control_write(app_dirname, "create", 0);
	exit(rc);

}

int main(int argc, char **argv)
{
	extern char *optarg;
	extern int optopt;
	int i, c;
	int iter = 0;
	unsigned opt_subbuf_size = 0;
	unsigned opt_n_subbufs = 0;
	unsigned opt_write_n = 0;
	unsigned opt_write_size = 0;
	unsigned opt_overwrite_mode = 0;
	unsigned opt_write_iter = 0;
	unsigned opt_expect_result = 0;
	unsigned opt_random_n = 0;
	unsigned opt_random_l = 0;
	unsigned opt_ignore_counts = 0;
	unsigned opt_kthread_secs = 0;
	size_t size, n;
	cpu_set_t cpu_mask;

	CPU_ZERO(&cpu_mask);
	CPU_SET(0, &cpu_mask);

	if (sched_setaffinity(0, sizeof(cpu_mask), &cpu_mask) == -1) {
		perror("sched_setaffinity");
		return -1;
	}

	while ((c = getopt(argc, argv, "b:n:w:s:o:i:e:r:c:l:t:")) != -1) {
		switch (c) {
		case 'b':
			opt_subbuf_size = (unsigned)atoi(optarg);
			if (!opt_subbuf_size)
				usage();
			break;
		case 'n':
			opt_n_subbufs = (unsigned)atoi(optarg);
			if (!opt_n_subbufs)
				usage();
			break;
		case 'w':
			opt_write_n = (unsigned)atoi(optarg);
			if (!opt_write_n)
				usage();
			break;
		case 's':
			opt_write_size = (unsigned)atoi(optarg);
			if (!opt_write_size)
				usage();
			break;
		case 'o':
			opt_overwrite_mode = (unsigned)atoi(optarg);
			if (!opt_overwrite_mode)
				usage();
			break;
		case 'i':
			opt_write_iter = (unsigned)atoi(optarg);
			if (!opt_write_iter)
				usage();
			break;
		case 'e':
			opt_expect_result = (unsigned)atoi(optarg);
			if (!opt_expect_result)
				usage();
			break;
		case 'r':
			opt_random_n = (unsigned)atoi(optarg);
			if (!opt_random_n)
				usage();
			break;
		case 'l':
			opt_random_l = (unsigned)atoi(optarg);
			if (!opt_random_l)
				usage();
			break;
		case 'c':
			opt_ignore_counts = (unsigned)atoi(optarg);
			if (!opt_ignore_counts)
				usage();
			break;
		case 't':
			opt_kthread_secs = (unsigned)atoi(optarg);
			if (!opt_kthread_secs)
				usage();
			break;
		case '?':
			printf("Unknown option -%c\n", optopt);
			usage();
			break;
		default:
			break;
		}
	}

	if ((opt_n_subbufs && !opt_subbuf_size) ||
	    (!opt_n_subbufs && opt_subbuf_size))
		usage();
	
	if (opt_n_subbufs && opt_n_subbufs) {
		subbuf_size = opt_subbuf_size;
		n_subbufs = opt_n_subbufs;
	}

	if (opt_write_n)
		write_n = opt_write_n;

	if (opt_write_size)
		write_size = opt_write_size;

	if (opt_overwrite_mode)
		overwrite_mode = opt_overwrite_mode;

	if (opt_write_iter)
		write_iter = opt_write_iter;

	if (opt_expect_result)
		expect_result = opt_expect_result;

	if (opt_random_n)
		random_n = opt_random_n;

	if (opt_random_l)
		random_l = opt_random_l;

	if (opt_ignore_counts)
		ignore_counts = opt_ignore_counts;

	if (opt_kthread_secs)
		kthread_secs = opt_kthread_secs;

	ncpus = sysconf(_SC_NPROCESSORS_ONLN);

	control_write(app_dirname, "subbuf_size", subbuf_size);
	control_write(app_dirname, "n_subbufs", n_subbufs);
	/* disable logging in case we exited badly in a previous run */
	control_write(app_dirname, "enabled", 0);
	control_write(app_dirname, "create", 1);

	signal(SIGINT, sigproc);
	signal(SIGTERM, sigproc);

	srand((unsigned)time(NULL));

	if (open_app_files())
		return -1;

	if (create_percpu_threads()) {
		close_app_files();
		return -1;
	}

	control_write(app_dirname, "enabled", 1);

	printf("Creating channel with %u sub-buffers of size %u.\n",
	       n_subbufs, subbuf_size);
	printf("Logging... Press Control-C to stop.\n");

	control_write(app_dirname, "overwrite", overwrite_mode);

	logging = 1;
	
	if (kthread_secs) {
		control_write(app_dirname, "event_n", random_n);
		control_write(app_dirname, "event_size", random_l);
		control_write(app_dirname, "start_test", kthread_secs);
	}

	while (1) {
		if (got_totals)
			sigproc(SIGTERM);

		if (kthread_secs)
			goto check_messages;
		
		if (iter < write_iter) {
			if (random_n)
				n = rand() / (RAND_MAX / random_n + 1);
			else
				n = write_n;
			if (random_l) {
				size = rand() / (RAND_MAX / random_l + 1);
				if (size < 10 && !ignore_counts)
					size = 10;
			} else
				size = write_size;
			
			if (size == 0) size = 1;
			if (n == 0) n = 1;
#if 0
			printf("writing %u records of size %u\n", n, size);
#endif
			control_write(app_dirname, "event_size", size);
			control_write(app_dirname, "event_n", n);
			control_write(app_dirname, "start_test", 0);
			/* for overwrite mode, we don't want to start
			   reading until writing is done */
			if ((iter == write_iter - 1) && overwrite_mode)
				done_writing = 1;
			wrote_n += n;
//			wrote_bytes += n * write_size;
			sleep(2);
			iter++;
		}
		
		if (iter == write_iter && !test_done) {
			test_done = 1;
			continue;
		}

check_messages:
		if (kthread_secs)
			test_done = control_read(app_dirname, "test_done");
		if (test_done) {
			control_write(app_dirname, "enabled", 0);
			kernel_wrote = control_read(app_dirname, "write_count");
			kernel_wrote_bytes = control_read_ll(app_dirname, "write_byte_count");
			kernel_lost = control_read(app_dirname, "dropped");
			got_totals = 1;
		}
	}
}


/* Boilerplate code below here */

/**
 *	reader_thread - per-cpu channel buffer reader
 */
static void *reader_thread(void *data)
{
	int rc, cpu = (int)data;
	unsigned seq;
	struct pollfd pollfd;
	char *buf = (char *)malloc(subbuf_size);

	do {
		pollfd.fd = relay_file[cpu];
		pollfd.events = POLLIN;
		rc = poll(&pollfd, 1, -1);
		if (rc < 0) {
			if (errno != EINTR) {
				printf("poll error: %s\n",strerror(errno));
				exit(1);
			}
			printf("poll warning: %s\n",strerror(errno));
		}
		/* for overwrite mode, we don't want to start reading
		   until writing is done */
		if (overwrite_mode && !done_writing)
			continue;
		rc = read(relay_file[cpu], buf, write_size);
		if (!rc)
			continue;
		if (rc < 0) {
			if (errno == EAGAIN)
				continue;
			perror("read");
			break;
		}
		if (!ignore_counts)
			sscanf(buf, "[%u", &seq);
#if 0
		printf("read %08u bytes: seq=%u\n", rc, seq);
#endif		
		if (!ignore_counts) {
			if(seq != prev_seq + 1) {
				if (seq > prev_seq) {
					missing += (seq - (prev_seq + 1));
					printf("gap: missing %u-%u\n", prev_seq + 1, seq - 1);
				} else if (prev_seq != -1)
					printf("repeat?: seq=%u, prev_seq=%u\n", seq, prev_seq);
			}
		}
		read_n++;
		read_bytes += rc;
		prev_seq = seq;
	} while (1);
}

static int summarize(void)
{
	int i, rc = 0;
	
	printf("summary:\n");
	printf("  subbuf_size:	%u\n", subbuf_size);
	printf("  n_subbufs:	%u\n", n_subbufs);
	printf("  overwrite:	%u\n", overwrite_mode);
	printf("  event size:	%u\n", write_size);
	printf("  read:		%u\n", read_n);
	printf("  wrote:	%u\n", kernel_wrote);
	printf("  read bytes:	%llu\n", read_bytes);
	printf("  wrote bytes:	%llu\n", kernel_wrote_bytes);
	printf("  lost:		%u\n", kernel_lost);

	if (expect_result) { /* override the calculated result */
		if (!ignore_counts) {
			printf("  expected:	%u\n", expect_result);
			printf("  got:		%u\n", read_n);
			if (expect_result == read_n)
				printf("  result:	pass\n");
			else {
				printf("  result:	FAIL\n");
				rc = 1;
			}
		} else {
			printf("  expected:	%u\n", expect_result);
			printf("  got:		%llu\n", read_bytes);
			if (expect_result == read_bytes)
				printf("  result:	pass\n");
			else {
				printf("  result:	FAIL\n");
				rc = 1;
			}
		}
	} else {
		if (!ignore_counts) {
			if (read_n + kernel_lost == wrote_n)
				printf("  read (%u) + lost (%u) == written (%u)\n",read_n, kernel_lost, wrote_n);
			else
				printf("  read (%u) + lost (%u) != written (%u)\n",read_n, kernel_lost, wrote_n);
		
			if (read_n + kernel_lost != wrote_n) {
				printf("  result:	FAIL\n");
				rc = 1;
			} else
				printf("  result:	pass\n");
		} else {
			if (kernel_wrote_bytes != read_bytes) {
				printf("  user byte_total (%llu) != kernel byte total (%llu)\n", read_bytes, kernel_wrote_bytes);
				printf("  result:	FAIL\n");
				rc = 1;
			} else {
				printf("  user byte_total (%llu) == kernel byte total (%llu)\n", read_bytes, kernel_wrote_bytes);
				printf("  result:	pass\n");
			}
		}
	}

	return rc;
}

/**
 *      create_percpu_threads - create per-cpu threads
 */
static int create_percpu_threads(void)
{
	int i;
	
	for (i = 0; i < ncpus; i++) {
		/* create a thread for each per-cpu buffer */
		if (pthread_create(&reader[i], NULL, reader_thread,
				   (void *)i) < 0) {
			printf("Couldn't create thread\n");
			control_write(app_dirname, "enabled", 0);
			control_write(app_dirname, "create", 0);
			return -1;
		}
	}

	return 0;
}

/**
 *	control_read - read a control file and return the value read
 */
static size_t control_read(const char *dirname,
			   const char *filename)
{
	char tmp[4096];
	int fd;

	sprintf(tmp, "%s/%s", dirname, filename);
	fd = open(tmp, O_RDONLY);
	if (fd < 0) {
		printf("Couldn't open control file %s\n", tmp);
		exit(1);
	}

	if (read(fd, tmp, sizeof(tmp)) < 0) {
		printf("Couldn't read control file %s: errcode = %d: %s\n",
		       tmp, errno, strerror(errno));
		close(fd);
		exit(1);
	}

	close(fd);

	return atoi(tmp);
}

/**
 *	control_read - read a control file and return the value read
 */
static size_t control_read_ll(const char *dirname,
			      const char *filename)
{
	char tmp[4096];
	int fd;

	sprintf(tmp, "%s/%s", dirname, filename);
	fd = open(tmp, O_RDONLY);
	if (fd < 0) {
		printf("Couldn't open control file %s\n", tmp);
		exit(1);
	}

	if (read(fd, tmp, sizeof(tmp)) < 0) {
		printf("Couldn't read control file %s: errcode = %d: %s\n",
		       tmp, errno, strerror(errno));
		close(fd);
		exit(1);
	}

	close(fd);

	return atoll(tmp);
}

/**
 *	control_read - write a value to a control file
 */
static void control_write(const char *dirname,
			  const char *filename,
			  size_t val)
{
	char tmp[4096];
	int fd;
	
	sprintf(tmp, "%s/%s", dirname, filename);
	fd = open(tmp, O_RDWR);
	if (fd < 0) {
		printf("Couldn't open control file %s\n", tmp);
		exit(1);
	}

	sprintf(tmp, "%zu", val);

	if (write(fd, tmp, strlen(tmp)) < 0) {
		printf("Couldn't write control file %s: errcode = %d: %s\n",
		       tmp, errno, strerror(errno));
		close(fd);
		exit(1);
	}

	close(fd);
}

/**
 *      kill_percpu_threads - kill per-cpu threads 0->n-1
 *      @n: number of threads to kill
 *
 *      Returns number of threads killed.
 */
static int kill_percpu_threads(int n)
{
        int i, killed = 0, err;
        
        for (i = 0; i < n; i++) {
                if ((err = pthread_cancel(reader[i])) == 0)
			killed++;
		else
			fprintf(stderr, "WARNING: couldn't kill per-cpu thread %d, err = %d\n", i, err);
        }
	
        if (killed != n)
                fprintf(stderr, "WARNING: couldn't kill all per-cpu threads:  %d killed, %d total\n", killed, n);

        return killed;
}

/**
 *	close_cpu_files - close and munmap buffer and open output file for cpu
 */
static void close_cpu_files(int cpu)
{
	close(relay_file[cpu]);
}

static void close_app_files(void)
{
	int i;
	
	for (i = 0; i < ncpus; i++)
		close_cpu_files(i);
}

/**
 *	open_files - open and mmap buffer and open output file
 */
static int open_cpu_files(int cpu, const char *dirname, const char *basename)
{
	char tmp[4096];

	sprintf(tmp, "%s/%s%d", dirname, basename, cpu);
	relay_file[cpu] = open(tmp, O_RDONLY | O_NONBLOCK);
	if (relay_file[cpu] < 0) {
		printf("Couldn't open relay file %s: errcode = %s\n",
		       tmp, strerror(errno));
		return -1;
	}

	return 0;
}

static int open_app_files(void)
{
	int i;

	for (i = 0; i < ncpus; i++) {
		if (open_cpu_files(i, app_dirname, percpu_basename) < 0) {
			control_write(app_dirname, "enabled", 0);
			control_write(app_dirname, "create", 0);
			return -1;
		}
	}

	return 0;
}


