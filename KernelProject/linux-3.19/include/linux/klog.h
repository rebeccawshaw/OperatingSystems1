/*
 *  klog debugging facility (see lib/klog.c)
 *
 * Copyright (C) IBM Corporation, 2005
 *
 * 2005		Tom Zanussi <zanussi@us.ibm.com>
 * 2003		Hubertus Franke
 *
 */
#ifndef _LINUX_KLOG_H
#define _LINUX_KLOG_H

/*
 * klog operations
 */
struct klog_operations
{
	/*
	 * klog - called when klog called, same params
	 */
	void (*klog) (const void *data, int len);
};

/*
 * klog functions
 */
extern int register_klog_handler(struct klog_operations *klog_ops);
extern void unregister_klog_handler(void);
extern void klog(const void *data, int len);
extern void klog_printk(const char *fmt, ...);

#endif /* _LINUX_KLOG_H */
