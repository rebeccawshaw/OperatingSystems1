/*
 *  readtest-mod - module to test read(2)
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
 * Copyright (C) IBM Corporation, 2005
 *
 * 2005-August	Created by Tom Zanussi <zanussi@us.ibm.com>
 *
 */
#include <linux/module.h>
#include <linux/relay.h>
#include <linux/debugfs.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/kthread.h>

/* This app's relay channel/control files will appear in /debug/readtest */
#define APP_DIR		"readtest"

/* app data */
static struct rchan *	chan;
static struct dentry *	dir;
static int		logging;
static int		flushed;
static int		mappings;
static int		suspended;
static size_t		dropped;
static size_t		subbuf_size = 262144;
static size_t		n_subbufs = 4;

/* channel-management control files */
static struct dentry	*enabled_control;
static struct dentry	*create_control;
static struct dentry	*subbuf_size_control;
static struct dentry	*n_subbufs_control;
static struct dentry	*dropped_control;

/* control file fileop declarations */
struct file_operations	enabled_fops;
struct file_operations	create_fops;
struct file_operations	subbuf_size_fops;
struct file_operations	n_subbufs_fops;
struct file_operations	dropped_fops;

/* forward declarations */
static void destroy_channel(void);
static void remove_controls(void);

/* readtest definitions */
#define MAX_EVENT_SIZE	256
#define MAX_N_EVENTS	1000000

static int overwrite_mode;
static char pattern[MAX_EVENT_SIZE];
static size_t write_count;
static unsigned long long write_byte_count;
static size_t cur_write_size;

static struct task_struct *kthread_thread;
//static task_t *kthread_thread;
static struct completion done;
static time_t kthread_run_secs;
static size_t event_n;
static size_t event_size;
static int test_done;

/* readtest-specific control files */
static struct dentry	*event_n_control;
static struct dentry	*event_size_control;
static struct dentry	*overwrite_control;
static struct dentry	*start_test_control;
static struct dentry	*end_test_control;
static struct dentry	*write_count_control;
static struct dentry	*write_byte_count_control;

/* readtest-specific control file fileop declarations */
struct file_operations	event_n_fops;
struct file_operations	event_size_fops;
struct file_operations	overwrite_fops;
struct file_operations	start_test_fops;
struct file_operations	end_test_fops;
struct file_operations	write_count_fops;
struct file_operations	write_byte_count_fops;

#if 0
static int event_size_counts[MAX_EVENT_SIZE + 1];
#endif

static inline int rand(void)
{
	int val;
	get_random_bytes(&val, sizeof(val));
	if (val < 0)
		val = -val;

	return val;
}

static inline int get_random_number(int max)
{
	int val = rand();
	return val / (INT_MAX / max + 1);
}

/*
 * relay_write, locally modified to write only to cpu 0
 */
static inline void relay_write_cpu_0(struct rchan *chan,
				     const void *data,
				     size_t length)
{
	unsigned long flags;
	struct rchan_buf *buf;

	local_irq_save(flags);
	buf = chan->buf[0];
	if (unlikely(buf->offset + length > chan->subbuf_size))
		length = relay_switch_subbuf(buf, length);
	memcpy(buf->data + buf->offset, data, length);
	buf->offset += length;
	local_irq_restore(flags);
}

static int test_thread(void *unused)
{
	int i, rand_n, rand_size;
	struct timeval start_time;
	struct timeval cur_time;
	time_t elapsed;
	char buf[MAX_EVENT_SIZE + 1];
	
	allow_signal(SIGKILL);
	allow_signal(SIGTERM);

	msleep(1000);

	do_gettimeofday(&start_time);

	while (!signal_pending(current)) {
		do_gettimeofday(&cur_time);
		elapsed = cur_time.tv_sec - start_time.tv_sec;
		if (elapsed < kthread_run_secs) {
			rand_n = get_random_number(event_n);
			if (!rand_n) rand_n = 1;
			for (i = 0; i < rand_n; i++) {
				rand_size = get_random_number(event_size);
				rand_size += 1;
				snprintf(buf, rand_size, "%s", pattern);
				cur_write_size = rand_size;
				if (logging)
					relay_write_cpu_0(chan, buf, rand_size);
#if 0
				event_size_counts[rand_size]++;
#endif
				write_count++;
				write_byte_count += rand_size;
			}
		} else
			break;
		
		if (kthread_should_stop())
			break;
		msleep(8);
	}

	complete(&done);
	test_done = 1;

	while(!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}
	
	return 0;
}

static void start_test_thread(void)
{
	int cpu = 0;
	struct task_struct *p;

	init_completion(&done);
	
	p = kthread_create(test_thread, NULL, "%s/%d", "test", cpu);
	if (IS_ERR(p))
		return;
	if (p) {
		kthread_bind(p, cpu);
		wake_up_process(p);
		kthread_thread = p;
	}
}

static void stop_test_thread(void)
{
	if (kthread_thread) {
		kthread_stop(kthread_thread);
		kthread_thread = NULL;
	}
}

static inline void random_test(void)
{
#if 0
	int i;
	
	for(i = 0; i <= MAX_EVENT_SIZE; i++)
		event_size_counts[i] = 0;
#endif
	if(!chan)
		return;

	start_test_thread();
}

static inline void n_test(void)
{
	int i;
	char buf[MAX_EVENT_SIZE + 1];
	
	if(!chan)
		return;

	for (i = 0; i < event_n; i++) {
		if (event_size < 10)
			snprintf(buf, event_size, "%s", pattern);
		else
			snprintf(buf, event_size, "[%08i]%s",
				 write_count, pattern);
		strcpy(buf + event_size - 1, "\n");
		cur_write_size = event_size;
		if (logging)
			relay_write_cpu_0(chan, buf, event_size);
		write_count++;
		write_byte_count += event_size;
	}
}

/**
 *	remove_readtest controls - removes readtest-specific control files
 */
static void remove_readtest_controls(void)
{
	if (event_n_control)
		debugfs_remove(event_n_control);

	if (event_size_control)
		debugfs_remove(event_size_control);

	if (overwrite_control)
		debugfs_remove(overwrite_control);

	if (start_test_control)
		debugfs_remove(start_test_control);

	if (end_test_control)
		debugfs_remove(end_test_control);

	if (write_count_control)
		debugfs_remove(write_count_control);

	if (write_byte_count_control)
		debugfs_remove(write_byte_count_control);
}

/**
 *	create_readtest_controls - creates readtest-specific control files
 *
 *	Returns 0 on success, negative otherwise.
 */
static int create_readtest_controls(void)
{
	event_n_control = debugfs_create_file("event_n", 0, dir,
					      NULL, &event_n_fops);
	if (!event_n_control) {
		printk("Couldn't create relay control file 'event_n'.\n");
		goto fail;
	}
	
	event_size_control = debugfs_create_file("event_size", 0, dir,
						 NULL, &event_size_fops);
	if (!event_size_control) {
		printk("Couldn't create relay control file 'event_size'.\n");
		goto fail;
	}

	overwrite_control = debugfs_create_file("overwrite", 0, dir,
						NULL, &overwrite_fops);
	if (!overwrite_control) {
		printk("Couldn't create relay control file 'overwrite'.\n");
		goto fail;
	}

	start_test_control = debugfs_create_file("start_test", 0, dir,
						 NULL, &start_test_fops);
	if (!start_test_control) {
		printk("Couldn't create relay control file 'start_test'.\n");
		goto fail;
	}

	end_test_control = debugfs_create_file("test_done", 0, dir,
					       NULL, &end_test_fops);
	if (!end_test_control) {
		printk("Couldn't create relay control file 'end_test'.\n");
		goto fail;
	}

	write_count_control = debugfs_create_file("write_count", 0, dir,
						  NULL, &write_count_fops);
	if (!write_count_control) {
		printk("Couldn't create relay control file 'write_count'.\n");
		goto fail;
	}

	write_byte_count_control = debugfs_create_file("write_byte_count", 0, dir, NULL, &write_byte_count_fops);
	if (!write_byte_count_control) {
		printk("Couldn't create relay control file 'write_byte_count'.\n");
		goto fail;
	}

	return 0;
fail:
	remove_readtest_controls();
	return -1;
}

static int create_controls(void);

/**
 *	module init - creates channel management control files
 *
 *	Returns 0 on success, negative otherwise.
 */
static int init(void)
{
	int i, j;

	dir = debugfs_create_dir(APP_DIR, NULL);
	if (!dir) {
		printk("Couldn't create relay app directory.\n");
		return -ENOMEM;
	}

	if (create_controls()) {
		debugfs_remove(dir);
		return -ENOMEM;
	}

	if (create_readtest_controls()) {
		remove_controls();
		debugfs_remove(dir);
		return -ENOMEM;
	}

	for (i = 0; i < MAX_EVENT_SIZE / 32; i++) {
		for (j = 0; j < 26; j++)
			pattern[i * 32 + j] = 'a' + j;
		for (j = 0; j < 6; j++)
			pattern[i * 32 + 26 + j] = '1' + j;
	}

	return 0;
}

static void cleanup(void)
{
	stop_test_thread();
	destroy_channel();
	remove_controls();
	remove_readtest_controls();
	if (dir)
		debugfs_remove(dir);
}

module_init(init);
module_exit(cleanup);
MODULE_LICENSE("GPL");


static ssize_t event_size_read(struct file *filp, char __user *buffer,
			       size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%zu\n", event_size);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t event_size_write(struct file *filp, const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	size_t n;
	
	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));
	
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	n = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	if (n > MAX_N_EVENTS)
		return -EINVAL;

	event_size = n;

	return count;
}

/*
 * 'event_size' file operations - r/w
 *
 *  gets/sets the size of events to write for the test
 */
struct file_operations event_size_fops = {
	.owner =	THIS_MODULE,
	.read =		event_size_read,
	.write =	event_size_write,
};

static ssize_t event_n_read(struct file *filp, char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%zu\n", event_n);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t event_n_write(struct file *filp, const char __user *buffer,
			     size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	size_t n;
	
	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));
	
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	n = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	if (n > MAX_N_EVENTS)
		return -EINVAL;

	event_n = n;

	return count;
}

/*
 * 'event_n' file operations - r/w
 *
 *  gets/sets the number of events to write for the test
 */
struct file_operations event_n_fops = {
	.owner =	THIS_MODULE,
	.read =		event_n_read,
	.write =	event_n_write,
};

static ssize_t overwrite_read(struct file *filp, char __user *buffer,
			      size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%d\n", overwrite_mode);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t overwrite_write(struct file *filp, const char __user *buffer,
			       size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	size_t n;
	
	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));
	
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	n = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	if (n > MAX_EVENT_SIZE)
		return -EINVAL;

	overwrite_mode = !!n;

	return count;
}

/*
 * 'overwrite_mode' file operations - r/w
 *
 *  gets/sets the overwrite_mode for the test
 */
struct file_operations overwrite_fops = {
	.owner =	THIS_MODULE,
	.read =		overwrite_read,
	.write =	overwrite_write,
};

static ssize_t start_test_write(struct file *filp, const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	size_t n;
	
	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));
	
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	n = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	kthread_run_secs = (time_t)n;

	if (!kthread_run_secs)
		n_test();
	else
		random_test();
	
	return count;
}

/*
 * 'start_test' file operations - w
 *
 *  starts the test.  If the value is 0, writes event_n events of size
 *  event_size.  If the value is nonzero, starts a kthread that will
 *  run kthread_run_secs, writing random numbers and sizes of events,
 *  where event_n and event_size are maximum limits.
 */
struct file_operations start_test_fops = {
	.owner =	THIS_MODULE,
	.write =	start_test_write,
};

static ssize_t end_test_read(struct file *filp, char __user *buffer,
			     size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%d\n", test_done);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

/*
 * 'end_test' file operations - r
 *
 *  returns whether the test is complete or not
 */
struct file_operations end_test_fops = {
	.owner =	THIS_MODULE,
	.read =		end_test_read,
};

static ssize_t write_count_read(struct file *filp, char __user *buffer,
				size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%zu\n", write_count);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

/*
 * 'write_count' file operations - r
 *
 *  gets the number of events written
 */
struct file_operations write_count_fops = {
	.owner =	THIS_MODULE,
	.read =		write_count_read,
};

static ssize_t write_byte_count_read(struct file *filp, char __user *buffer,
				     size_t count, loff_t *ppos)
{
	char buf[32];
	
	snprintf(buf, sizeof(buf), "%llu\n", write_byte_count);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

/*
 * 'write_byte_count' file operations - r
 *
 *  gets the number of bytes written
 */
struct file_operations write_byte_count_fops = {
	.owner =	THIS_MODULE,
	.read =		write_byte_count_read,
};


/*
 * subbuf_start() relay callback.
 *
 * Defined so that we can 1) reserve padding counts in the sub-buffers, and
 * 2) keep a count of events dropped due to the buffer-full condition.
 */
static int subbuf_start_handler(struct rchan_buf *buf,
				  void *subbuf,
				  void *prev_subbuf,
				  unsigned int prev_padding)
{
	if (overwrite_mode)
		return 1;
	
	if (relay_buf_full(buf)) {
		if (!suspended) {
			suspended = 1;
			printk("cpu %d buffer full!!!\n", smp_processor_id());
		}
		dropped++;
		write_byte_count -= cur_write_size;
		return 0;
	} else if (suspended) {
		suspended = 0;
		printk("cpu %d buffer no longer full.\n",smp_processor_id());
	}

	return 1;
}

/*
 * file_create() callback.  Creates relay file in debugfs.
 */
static struct dentry *create_buf_file_handler(const char *filename,
					      struct dentry *parent,
					      int mode,
					      struct rchan_buf *buf,
					      int *is_global)
{
	struct dentry *buf_file;
	
	buf_file = debugfs_create_file(filename, mode, parent, buf,
				       &relay_file_operations);

	return buf_file;
}

/*
 * file_remove() default callback.  Removes relay file in debugfs.
 */
static int remove_buf_file_handler(struct dentry *dentry)
{
	debugfs_remove(dentry);

	return 0;
}

/* Boilerplate code below here */

/*
 * relay callbacks
 */
static struct rchan_callbacks relay_callbacks =
{
	.subbuf_start = subbuf_start_handler,
	.create_buf_file = create_buf_file_handler,
	.remove_buf_file = remove_buf_file_handler,
};

/**
 *	create_channel - creates channel /debug/readtest/cpuXXX
 *
 *	Creates channel
 *
 *	Returns channel on success, NULL otherwise
 */
static struct rchan *create_channel(unsigned subbuf_size,
				    unsigned n_subbufs)
{
	struct rchan *chan;

	chan = relay_open("cpu", dir, subbuf_size,
			  n_subbufs, &relay_callbacks, NULL);
	
	if (!chan) {
		printk("relay app channel creation failed\n");
		return NULL;
	}

	logging = 0;
	mappings = 0;
	suspended = 0;
	dropped = 0;
	write_count = 0;
	write_byte_count = 0;
	test_done = 0;

	return chan;
}

/**
 *	destroy_channel - destroys channel /mnt/relay/readtest/cpuXXX
 *
 *	Destroys channel
 */
static void destroy_channel(void)
{
	if (chan) {
		relay_close(chan);
		chan = NULL;
	}
}

/**
 *	remove_controls - removes channel management control files
 */
static void remove_controls(void)
{
	if (enabled_control)
		debugfs_remove(enabled_control);

	if (subbuf_size_control)
		debugfs_remove(subbuf_size_control);

	if (n_subbufs_control)
		debugfs_remove(n_subbufs_control);

	if (create_control)
		debugfs_remove(create_control);

	if (dropped_control)
		debugfs_remove(dropped_control);
}

/**
 *	create_controls - creates channel management control files
 *
 *	Returns 0 on success, negative otherwise.
 */
static int create_controls(void)
{
	enabled_control = debugfs_create_file("enabled", 0, dir,
					      NULL, &enabled_fops);
	if (!enabled_control) {
		printk("Couldn't create relay control file 'enabled'.\n");
		goto fail;
	}
	
	subbuf_size_control = debugfs_create_file("subbuf_size", 0, dir,
						  NULL, &subbuf_size_fops);
	if (!subbuf_size_control) {
		printk("Couldn't create relay control file 'subbuf_size'.\n");
		goto fail;
	}

	n_subbufs_control = debugfs_create_file("n_subbufs", 0, dir,
						NULL, &n_subbufs_fops);
	if (!n_subbufs_control) {
		printk("Couldn't create relay control file 'n_subbufs'.\n");
		goto fail;
	}

	create_control = debugfs_create_file("create", 0, dir,
					     NULL, &create_fops);
	if (!create_control) {
		printk("Couldn't create relay control file 'create'.\n");
		goto fail;
	}

	dropped_control = debugfs_create_file("dropped", 0, dir,
					      NULL, &dropped_fops);
	if (!dropped_control) {
		printk("Couldn't create relay control file 'dropped'.\n");
		goto fail;
	}

	return 0;
fail:
	remove_controls();
	return -1;
}

/*
 * control file fileop definitions
 */

/*
 * control files for relay channel management
 */

static ssize_t enabled_read(struct file *filp, char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%d\n", logging);
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t enabled_write(struct file *filp, const char __user *buffer,
			     size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	int enabled;
	
	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));
	
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	enabled = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	if (enabled && chan) {
		flushed = 0;
		logging = 1;
	} else if (!enabled) {
		logging = 0;
		if (chan) {
			flushed = 1;
			relay_flush(chan);
		}
		if (kthread_thread) {
			stop_test_thread();
#if 0
			{
				int i;
				for(i = 0; i < kthread_size + 1; i++)
					printk("  event_size %d: %d\n",
					       i, event_size_counts[i]);
			}
#endif
		}
	}
		
	return count;
}

/*
 * 'enabled' file operations - boolean r/w
 *
 *  toggles logging to the relay channel
 */
struct file_operations enabled_fops = {
	.owner =	THIS_MODULE,
	.read =		enabled_read,
	.write =	enabled_write,
};

static ssize_t create_read(struct file *filp, char __user *buffer,
			   size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%d\n", !!chan);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t create_write(struct file *filp, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	int create;
	
	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));
	
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	create = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	if (create) {
		destroy_channel();
		chan = create_channel(subbuf_size, n_subbufs);
		if(!chan)
			return count;
	} else
		destroy_channel();
		
	return count;
}

/*
 * 'create' file operations - boolean r/w
 *
 *  creates/destroys the relay channel
 */
struct file_operations create_fops = {
	.owner =	THIS_MODULE,
	.read =		create_read,
	.write =	create_write,
};

static ssize_t subbuf_size_read(struct file *filp, char __user *buffer,
				size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%zu\n", subbuf_size);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t subbuf_size_write(struct file *filp, const char __user *buffer,
				 size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	size_t size;
	
	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));
	
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	size = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	subbuf_size = size;

	return count;
}

/*
 * 'subbuf_size' file operations - r/w
 *
 *  gets/sets the subbuffer size to use in channel creation
 */
struct file_operations subbuf_size_fops = {
	.owner =	THIS_MODULE,
	.read =		subbuf_size_read,
	.write =	subbuf_size_write,
};

static ssize_t n_subbufs_read(struct file *filp, char __user *buffer,
			      size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%zu\n", n_subbufs);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t n_subbufs_write(struct file *filp, const char __user *buffer,
			       size_t count, loff_t *ppos)
{
	char buf[16];
	char *tmp;
	size_t n;
	
	if (count > sizeof(buf))
		return -EINVAL;

	memset(buf, 0, sizeof(buf));
	
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	n = simple_strtol(buf, &tmp, 10);
	if (tmp == buf)
		return -EINVAL;

	n_subbufs = n;

	return count;
}

/*
 * 'n_subbufs' file operations - r/w
 *
 *  gets/sets the number of subbuffers to use in channel creation
 */
struct file_operations n_subbufs_fops = {
	.owner =	THIS_MODULE,
	.read =		n_subbufs_read,
	.write =	n_subbufs_write,
};

static ssize_t dropped_read(struct file *filp, char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[16];
	
	snprintf(buf, sizeof(buf), "%zu\n", dropped);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

/*
 * 'dropped' file operations - r
 *
 *  gets the number of dropped events seen
 */
struct file_operations dropped_fops = {
	.owner =	THIS_MODULE,
	.read =		dropped_read,
};
