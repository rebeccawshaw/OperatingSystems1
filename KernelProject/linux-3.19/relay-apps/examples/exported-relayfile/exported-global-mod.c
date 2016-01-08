/*
 *  exported-global-mod - test exported relay file ops with a global buffer
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
 * 2005		Tom Zanussi <zanussi@us.ibm.com>
 *
 */

#include <linux/module.h>
#include <linux/relay.h>
#include <linux/debugfs.h>

/* This app's relayfs channel will be /debug/exported-global/global0 */
#define APP_DIR		"exported-global"

/* app data */
static struct rchan *	chan;
static struct dentry *	dir;
static size_t		subbuf_size = 262144;
static size_t		n_subbufs = 4;
static DEFINE_SPINLOCK(chan_lock);

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
	*is_global = 1;

	return buf_file;
}

/*
 * file_remove() default callback.  Removes relay file in debugfs.
 */
static int remove_buf_file_handler(struct dentry *dentry)
{
	printk("remove_buf_file_handler: dentry %p\n", dentry);
	
	debugfs_remove(dentry);

	return 0;
}

/*
 * relayfs callbacks
 */
static struct rchan_callbacks relayfs_callbacks =
{
	.create_buf_file = create_buf_file_handler,
	.remove_buf_file = remove_buf_file_handler,
};


/**
 *	module init - creates channel management control files
 *
 *	Returns 0 on success, negative otherwise.
 */
static int init(void)
{
	unsigned long flags;

	dir = debugfs_create_dir(APP_DIR, NULL);
	if (!dir) {
		printk("Couldn't create debugfs app directory.\n");
		return -ENOMEM;
	}

	chan = relay_open("global", dir, subbuf_size,
			  n_subbufs, &relayfs_callbacks, NULL);
	
	if (!chan) {
		printk("relay app channel creation failed\n");
		debugfs_remove(dir);
		return -ENOMEM;
	}

	/* just for testing - log some data from this module */
	{
		int i;
		char buf[64];

		for (i = 0; i <= 123; i++) {
			sprintf(buf, "testing %d.  testing.\n", i);
			spin_lock_irqsave(&chan_lock, flags);
			relay_write(chan, buf, strlen(buf));
			spin_unlock_irqrestore(&chan_lock, flags);
		}
	}

	return 0;
}

static void cleanup(void)
{
	printk("exported-global-mod: cleanup, chan %p\n", chan);
	
	if (chan)
		relay_close(chan);
	if (dir)
		debugfs_remove(dir);
}

module_init(init);
module_exit(cleanup);
MODULE_LICENSE("GPL");
