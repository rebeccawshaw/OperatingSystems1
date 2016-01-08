/*
 *  klog debugging facility
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
 * 2003		Hubertus Franke
 *
 * klog is a debugging facility built on top of relayfs.  If you
 * configure KLOG in (say 'y' to 'klog debugging functions' in the
 * 'kernel hacking' config section), you can call klog() and
 * klog_printk() from anywhere in the kernel or kernel modules,
 * regardless of whether there's a 'handler' actually writing the data
 * to a relayfs channel.  To do something with the data e.g. have it
 * logged to a relayfs channel, create your own module and register a
 * klog handler to handle the klog data and write it to a relayfs
 * channel (see lib/klog-simple.c for an example).  Use
 * register_klog_handler() and unregister_klog_handler() to start/stop
 * having the logged data sent to your module.
 */
#include <linux/module.h>
#include <linux/klog.h>

/* maximum size of klog formatting buffer beyond which truncation will occur */
#define KLOG_TMPBUF_SIZE (1024)
/* per-cpu klog formatting temporary buffer */
static char klog_buf[NR_CPUS][KLOG_TMPBUF_SIZE];

/*
 * do-nothing default klog handler, called if nothing registered
 */
static void default_klog(const void *data, int len)
{
}

/*
 * default klog operations, used if nothing registered
 */
static struct klog_operations default_klog_ops =
{
	.klog = default_klog,
};

static struct klog_operations *cur_klog_ops = &default_klog_ops;

/**
 *	register_klog_handler - register klog handler
 *	@klog_ops: klog operations callbacks
 *
 *	replaces default klog handler with passed-in version
 */
int register_klog_handler(struct klog_operations *klog_ops)
{
	if (!klog_ops)
		return -EINVAL;
	
	if (!klog_ops->klog)
		klog_ops->klog = default_klog;

	cur_klog_ops = klog_ops;

	return 0;
}

/**
 *	unregister_klog_handler - unregister klog handler
 *
 *	default handler will be in effect after this
 */
void unregister_klog_handler(void)
{
	cur_klog_ops = &default_klog_ops;
}

/**
 *	klog - send raw data to klog handler
 */
void klog(const void *data, int len)
{
	cur_klog_ops->klog(data, len);
}

/**
 *	klog_printk - send a formatted string to the klog handler
 *	@fmt: format string, same as printk
 */
void klog_printk(const char *fmt, ...)
{
	va_list args;
	int len;
	char *cbuf;
	unsigned long flags;

	local_irq_save(flags);
	cbuf = klog_buf[smp_processor_id()];
	va_start(args, fmt);
	len = vsnprintf(cbuf, KLOG_TMPBUF_SIZE, fmt, args);
	va_end(args);
	klog(cbuf, len);
	local_irq_restore(flags);
}

EXPORT_SYMBOL_GPL(klog);
EXPORT_SYMBOL_GPL(klog_printk);
EXPORT_SYMBOL_GPL(register_klog_handler);
EXPORT_SYMBOL_GPL(unregister_klog_handler);
