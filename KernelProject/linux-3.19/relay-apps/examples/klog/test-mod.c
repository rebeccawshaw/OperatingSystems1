/*
 *  test-mod - log test data
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
#include <linux/klog.h>

static int init(void)
{
	int i;

	/* log some test data from this module */
	for (i = 0; i <= 123; i++)
//		klog_printk("testing %d.  testing.\n", i);
		printk("testing %d.  testing.\n", i);

	return 0;
}

static void cleanup(void)
{
}

module_init(init);
module_exit(cleanup);
MODULE_LICENSE("GPL");
