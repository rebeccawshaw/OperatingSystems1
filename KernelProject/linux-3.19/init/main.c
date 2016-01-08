/*
 *  linux/init/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  GK 2/5/95  -  Changed to support mounting root fs via NFS
 *  Added initrd & change_root: Werner Almesberger & Hans Lermen, Feb '96
 *  Moan early if gcc is old, avoiding bogus kernels - Paul Gortmaker, May '96
 *  Simplified starting of init:  Michael A. Griffith <grif@acm.org>
 */

#define DEBUG		/* Enable initcall_debug */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/stackprotector.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/bootmem.h>
#include <linux/acpi.h>
#include <linux/tty.h>
#include <linux/percpu.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/kernel_stat.h>
#include <linux/start_kernel.h>
#include <linux/security.h>
#include <linux/smp.h>
#include <linux/profile.h>
#include <linux/rcupdate.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>
#include <linux/writeback.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/cgroup.h>
#include <linux/efi.h>
#include <linux/tick.h>
#include <linux/interrupt.h>
#include <linux/taskstats_kern.h>
#include <linux/delayacct.h>
#include <linux/unistd.h>
#include <linux/rmap.h>
#include <linux/mempolicy.h>
#include <linux/key.h>
#include <linux/buffer_head.h>
#include <linux/page_ext.h>
#include <linux/debug_locks.h>
#include <linux/debugobjects.h>
#include <linux/lockdep.h>
#include <linux/kmemleak.h>
#include <linux/pid_namespace.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/idr.h>
#include <linux/kgdb.h>
#include <linux/ftrace.h>
#include <linux/async.h>
#include <linux/kmemcheck.h>
#include <linux/sfi.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/perf_event.h>
#include <linux/file.h>
#include <linux/ptrace.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/sched_clock.h>
#include <linux/context_tracking.h>
#include <linux/random.h>
#include <linux/list.h>
#include <linux/integrity.h>
#include <linux/proc_ns.h>

#include <asm/io.h>
#include <asm/bugs.h>
#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/cacheflush.h>

#ifdef CONFIG_X86_LOCAL_APIC
#include <asm/smp.h>
#endif

#include <linux/relay.h>
#include <linux/debugfs.h>

/* This app's relayfs channel/control files will appear in
   /debug/exported-relayfile */
#define APP_DIR		"kleak"

/* app data */
struct rchan *	chan;
static struct dentry *	dir;
static int		logging;
static int		mappings;
static int		suspended;
static size_t		dropped;
static size_t		subbuf_size = 64;
static size_t		n_subbufs = 10;

/* channel-management control files */
static struct dentry	*enabled_control;
static struct dentry	*create_control;
static struct dentry	*subbuf_size_control;
static struct dentry	*n_subbufs_control;
static struct dentry	*dropped_control;

/* produced/consumed control files */
static struct dentry	*produced_control[NR_CPUS];
static struct dentry	*consumed_control[NR_CPUS];

/* control file fileop declarations */
struct file_operations	enabled_fops;
struct file_operations	create_fops;
struct file_operations	subbuf_size_fops;
struct file_operations	n_subbufs_fops;
struct file_operations	dropped_fops;
struct file_operations	produced_fops;
struct file_operations	consumed_fops;

static int kernel_init(void *);

extern void init_IRQ(void);
extern void fork_init(unsigned long);
extern void radix_tree_init(void);
#ifndef CONFIG_DEBUG_RODATA
static inline void mark_rodata_ro(void) { }
#endif

/*
 * Debug helper: via this flag we know that we are in 'early bootup code'
 * where only the boot processor is running with IRQ disabled.  This means
 * two things - IRQ must not be enabled before the flag is cleared and some
 * operations which are not allowed with IRQ disabled are allowed while the
 * flag is set.
 */
bool early_boot_irqs_disabled __read_mostly;

enum system_states system_state __read_mostly;
EXPORT_SYMBOL(system_state);

/*
 * Boot command-line arguments
 */
#define MAX_INIT_ARGS CONFIG_INIT_ENV_ARG_LIMIT
#define MAX_INIT_ENVS CONFIG_INIT_ENV_ARG_LIMIT

extern void time_init(void);
/* Default late time init is NULL. archs can override this later. */
void (*__initdata late_time_init)(void);

/* Untouched command line saved by arch-specific code. */
char __initdata boot_command_line[COMMAND_LINE_SIZE];
/* Untouched saved command line (eg. for /proc) */
char *saved_command_line;
/* Command line for parameter parsing */
static char *static_command_line;
/* Command line for per-initcall parameter parsing */
static char *initcall_command_line;

static char *execute_command;
static char *ramdisk_execute_command;

/*
 * Used to generate warnings if static_key manipulation functions are used
 * before jump_label_init is called.
 */
bool static_key_initialized __read_mostly;
EXPORT_SYMBOL_GPL(static_key_initialized);

/*
 * If set, this is an indication to the drivers that reset the underlying
 * device before going ahead with the initialization otherwise driver might
 * rely on the BIOS and skip the reset operation.
 *
 * This is useful if kernel is booting in an unreliable environment.
 * For ex. kdump situaiton where previous kernel has crashed, BIOS has been
 * skipped and devices will be in unknown state.
 */
unsigned int reset_devices;
EXPORT_SYMBOL(reset_devices);

static int __init set_reset_devices(char *str)
{
	reset_devices = 1;
	return 1;
}

__setup("reset_devices", set_reset_devices);

static const char *argv_init[MAX_INIT_ARGS+2] = { "init", NULL, };
const char *envp_init[MAX_INIT_ENVS+2] = { "HOME=/", "TERM=linux", NULL, };
static const char *panic_later, *panic_param;

extern const struct obs_kernel_param __setup_start[], __setup_end[];

static int __init obsolete_checksetup(char *line)
{
	const struct obs_kernel_param *p;
	int had_early_param = 0;

	p = __setup_start;
	do {
		int n = strlen(p->str);
		if (parameqn(line, p->str, n)) {
			if (p->early) {
				/* Already done in parse_early_param?
				 * (Needs exact match on param part).
				 * Keep iterating, as we can have early
				 * params and __setups of same names 8( */
				if (line[n] == '\0' || line[n] == '=')
					had_early_param = 1;
			} else if (!p->setup_func) {
				pr_warn("Parameter %s is obsolete, ignored\n",
					p->str);
				return 1;
			} else if (p->setup_func(line + n))
				return 1;
		}
		p++;
	} while (p < __setup_end);

	return had_early_param;
}

/*
 * This should be approx 2 Bo*oMips to start (note initial shift), and will
 * still work even if initially too large, it will just take slightly longer
 */
unsigned long loops_per_jiffy = (1<<12);
EXPORT_SYMBOL(loops_per_jiffy);

static int __init debug_kernel(char *str)
{
	console_loglevel = CONSOLE_LOGLEVEL_DEBUG;
	return 0;
}

static int __init quiet_kernel(char *str)
{
	console_loglevel = CONSOLE_LOGLEVEL_QUIET;
	return 0;
}

early_param("debug", debug_kernel);
early_param("quiet", quiet_kernel);

static int __init loglevel(char *str)
{
	int newlevel;

	/*
	 * Only update loglevel value when a correct setting was passed,
	 * to prevent blind crashes (when loglevel being set to 0) that
	 * are quite hard to debug
	 */
	if (get_option(&str, &newlevel)) {
		console_loglevel = newlevel;
		return 0;
	}

	return -EINVAL;
}

early_param("loglevel", loglevel);

/* Change NUL term back to "=", to make "param" the whole string. */
static int __init repair_env_string(char *param, char *val, const char *unused)
{
	if (val) {
		/* param=val or param="val"? */
		if (val == param+strlen(param)+1)
			val[-1] = '=';
		else if (val == param+strlen(param)+2) {
			val[-2] = '=';
			memmove(val-1, val, strlen(val)+1);
			val--;
		} else
			BUG();
	}
	return 0;
}

/* Anything after -- gets handed straight to init. */
static int __init set_init_arg(char *param, char *val, const char *unused)
{
	unsigned int i;

	if (panic_later)
		return 0;

	repair_env_string(param, val, unused);

	for (i = 0; argv_init[i]; i++) {
		if (i == MAX_INIT_ARGS) {
			panic_later = "init";
			panic_param = param;
			return 0;
		}
	}
	argv_init[i] = param;
	return 0;
}

/*
 * Unknown boot options get handed to init, unless they look like
 * unused parameters (modprobe will find them in /proc/cmdline).
 */
static int __init unknown_bootoption(char *param, char *val, const char *unused)
{
	repair_env_string(param, val, unused);

	/* Handle obsolete-style parameters */
	if (obsolete_checksetup(param))
		return 0;

	/* Unused module parameter. */
	if (strchr(param, '.') && (!val || strchr(param, '.') < val))
		return 0;

	if (panic_later)
		return 0;

	if (val) {
		/* Environment option */
		unsigned int i;
		for (i = 0; envp_init[i]; i++) {
			if (i == MAX_INIT_ENVS) {
				panic_later = "env";
				panic_param = param;
			}
			if (!strncmp(param, envp_init[i], val - param))
				break;
		}
		envp_init[i] = param;
	} else {
		/* Command line option */
		unsigned int i;
		for (i = 0; argv_init[i]; i++) {
			if (i == MAX_INIT_ARGS) {
				panic_later = "init";
				panic_param = param;
			}
		}
		argv_init[i] = param;
	}
	return 0;
}

static int __init init_setup(char *str)
{
	unsigned int i;

	execute_command = str;
	/*
	 * In case LILO is going to boot us with default command line,
	 * it prepends "auto" before the whole cmdline which makes
	 * the shell think it should execute a script with such name.
	 * So we ignore all arguments entered _before_ init=... [MJ]
	 */
	for (i = 1; i < MAX_INIT_ARGS; i++)
		argv_init[i] = NULL;
	return 1;
}
__setup("init=", init_setup);

static int __init rdinit_setup(char *str)
{
	unsigned int i;

	ramdisk_execute_command = str;
	/* See "auto" comment in init_setup */
	for (i = 1; i < MAX_INIT_ARGS; i++)
		argv_init[i] = NULL;
	return 1;
}
__setup("rdinit=", rdinit_setup);

#ifndef CONFIG_SMP
static const unsigned int setup_max_cpus = NR_CPUS;
#ifdef CONFIG_X86_LOCAL_APIC
static void __init smp_init(void)
{
	APIC_init_uniprocessor();
}
#else
#define smp_init()	do { } while (0)
#endif

static inline void setup_nr_cpu_ids(void) { }
static inline void smp_prepare_cpus(unsigned int maxcpus) { }
#endif

/*
 * We need to store the untouched command line for future reference.
 * We also need to store the touched command line since the parameter
 * parsing is performed in place, and we should allow a component to
 * store reference of name/value for future reference.
 */
static void __init setup_command_line(char *command_line)
{
	saved_command_line =
		memblock_virt_alloc(strlen(boot_command_line) + 1, 0);
	initcall_command_line =
		memblock_virt_alloc(strlen(boot_command_line) + 1, 0);
	static_command_line = memblock_virt_alloc(strlen(command_line) + 1, 0);
	strcpy(saved_command_line, boot_command_line);
	strcpy(static_command_line, command_line);
}

/*
 * We need to finalize in a non-__init function or else race conditions
 * between the root thread and the init thread may cause start_kernel to
 * be reaped by free_initmem before the root thread has proceeded to
 * cpu_idle.
 *
 * gcc-3.4 accidentally inlines this function, so use noinline.
 */

static __initdata DECLARE_COMPLETION(kthreadd_done);

static noinline void __init_refok rest_init(void)
{
	int pid;

	rcu_scheduler_starting();
	/*
	 * We need to spawn init first so that it obtains pid 1, however
	 * the init task will end up wanting to create kthreads, which, if
	 * we schedule it before we create kthreadd, will OOPS.
	 */
	kernel_thread(kernel_init, NULL, CLONE_FS);
	numa_default_policy();
	pid = kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES);
	rcu_read_lock();
	kthreadd_task = find_task_by_pid_ns(pid, &init_pid_ns);
	rcu_read_unlock();
	complete(&kthreadd_done);

	/*
	 * The boot idle thread must execute schedule()
	 * at least once to get things moving:
	 */
	init_idle_bootup_task(current);
	schedule_preempt_disabled();
	/* Call into cpu_idle with preempt disabled */
	cpu_startup_entry(CPUHP_ONLINE);
}

/* Check for early params. */
static int __init do_early_param(char *param, char *val, const char *unused)
{
	const struct obs_kernel_param *p;

	for (p = __setup_start; p < __setup_end; p++) {
		if ((p->early && parameq(param, p->str)) ||
		    (strcmp(param, "console") == 0 &&
		     strcmp(p->str, "earlycon") == 0)
		) {
			if (p->setup_func(val) != 0)
				pr_warn("Malformed early option '%s'\n", param);
		}
	}
	/* We accept everything at this stage. */
	return 0;
}

void __init parse_early_options(char *cmdline)
{
	parse_args("early options", cmdline, NULL, 0, 0, 0, do_early_param);
}

/* Arch code calls this early on, or if not, just before other parsing. */
void __init parse_early_param(void)
{
	static int done __initdata;
	static char tmp_cmdline[COMMAND_LINE_SIZE] __initdata;

	if (done)
		return;

	/* All fall through to do_early_param. */
	strlcpy(tmp_cmdline, boot_command_line, COMMAND_LINE_SIZE);
	parse_early_options(tmp_cmdline);
	done = 1;
}

/*
 *	Activate the first processor.
 */

static void __init boot_cpu_init(void)
{
	int cpu = smp_processor_id();
	/* Mark the boot cpu "present", "online" etc for SMP and UP case */
	set_cpu_online(cpu, true);
	set_cpu_active(cpu, true);
	set_cpu_present(cpu, true);
	set_cpu_possible(cpu, true);
}

void __init __weak smp_setup_processor_id(void)
{
}

# if THREAD_SIZE >= PAGE_SIZE
void __init __weak thread_info_cache_init(void)
{
}
#endif

/*
 * Set up kernel memory allocators
 */
static void __init mm_init(void)
{
	/*
	 * page_ext requires contiguous pages,
	 * bigger than MAX_ORDER unless SPARSEMEM.
	 */
	page_ext_init_flatmem();
	mem_init();
	kmem_cache_init();
	percpu_init_late();
	pgtable_init();
	vmalloc_init();
}

asmlinkage __visible void __init start_kernel(void)
{
	char *command_line;
	char *after_dashes;

	/*
	 * Need to run as early as possible, to initialize the
	 * lockdep hash:
	 */
	lockdep_init();
	set_task_stack_end_magic(&init_task);
	smp_setup_processor_id();
	debug_objects_early_init();

	/*
	 * Set up the the initial canary ASAP:
	 */
	boot_init_stack_canary();

	cgroup_init_early();

	local_irq_disable();
	early_boot_irqs_disabled = true;

/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */
	boot_cpu_init();
	page_address_init();
	pr_notice("%s", linux_banner);
	setup_arch(&command_line);
	mm_init_cpumask(&init_mm);
	setup_command_line(command_line);
	setup_nr_cpu_ids();
	setup_per_cpu_areas();
	smp_prepare_boot_cpu();	/* arch-specific boot-cpu hooks */

	build_all_zonelists(NULL, NULL);
	page_alloc_init();

	pr_notice("Kernel command line: %s\n", boot_command_line);
	parse_early_param();
	after_dashes = parse_args("Booting kernel",
				  static_command_line, __start___param,
				  __stop___param - __start___param,
				  -1, -1, &unknown_bootoption);
	if (!IS_ERR_OR_NULL(after_dashes))
		parse_args("Setting init args", after_dashes, NULL, 0, -1, -1,
			   set_init_arg);

	jump_label_init();

	/*
	 * These use large bootmem allocations and must precede
	 * kmem_cache_init()
	 */
	setup_log_buf(0);
	pidhash_init();
	vfs_caches_init_early();
	sort_main_extable();
	trap_init();
	mm_init();

	/*
	 * Set up the scheduler prior starting any interrupts (such as the
	 * timer interrupt). Full topology setup happens at smp_init()
	 * time - but meanwhile we still have a functioning scheduler.
	 */
	sched_init();
	/*
	 * Disable preemption - early bootup scheduling is extremely
	 * fragile until we cpu_idle() for the first time.
	 */
	preempt_disable();
	if (WARN(!irqs_disabled(),
		 "Interrupts were enabled *very* early, fixing it\n"))
		local_irq_disable();
	idr_init_cache();
	rcu_init();

	/* trace_printk() and trace points may be used after this */
	trace_init();

	context_tracking_init();
	radix_tree_init();
	/* init some links before init_ISA_irqs() */
	early_irq_init();
	init_IRQ();
	tick_init();
	rcu_init_nohz();
	init_timers();
	hrtimers_init();
	softirq_init();
	timekeeping_init();
	time_init();
	sched_clock_postinit();
	perf_event_init();
	profile_init();
	call_function_init();
	WARN(!irqs_disabled(), "Interrupts were enabled early\n");
	early_boot_irqs_disabled = false;
	local_irq_enable();

	kmem_cache_init_late();

	/*
	 * HACK ALERT! This is early. We're enabling the console before
	 * we've done PCI setups etc, and console_init() must be aware of
	 * this. But we do want output early, in case something goes wrong.
	 */
	console_init();
	if (panic_later)
		panic("Too many boot %s vars at `%s'", panic_later,
		      panic_param);

	lockdep_info();

	/*
	 * Need to run this when irqs are enabled, because it wants
	 * to self-test [hard/soft]-irqs on/off lock inversion bugs
	 * too:
	 */
	locking_selftest();

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start && !initrd_below_start_ok &&
	    page_to_pfn(virt_to_page((void *)initrd_start)) < min_low_pfn) {
		pr_crit("initrd overwritten (0x%08lx < 0x%08lx) - disabling it.\n",
		    page_to_pfn(virt_to_page((void *)initrd_start)),
		    min_low_pfn);
		initrd_start = 0;
	}
#endif
	page_ext_init();
	debug_objects_mem_init();
	kmemleak_init();
	setup_per_cpu_pageset();
	numa_policy_init();
	if (late_time_init)
		late_time_init();
	sched_clock_init();
	calibrate_delay();
	pidmap_init();
	anon_vma_init();
	acpi_early_init();
#ifdef CONFIG_X86
	if (efi_enabled(EFI_RUNTIME_SERVICES))
		efi_enter_virtual_mode();
#endif
#ifdef CONFIG_X86_ESPFIX64
	/* Should be run before the first non-init thread is created */
	init_espfix_bsp();
#endif
	thread_info_cache_init();
	cred_init();
	fork_init(totalram_pages);
	proc_caches_init();
	buffer_init();
	key_init();
	security_init();
	dbg_late_init();
	vfs_caches_init(totalram_pages);
	signals_init();
	/* rootfs populating might need page-writeback */
	page_writeback_init();
	proc_root_init();
	nsfs_init();
	cgroup_init();
	cpuset_init();
	taskstats_init_early();
	delayacct_init();

	check_bugs();

	sfi_init_late();

	if (efi_enabled(EFI_RUNTIME_SERVICES)) {
		efi_late_init();
		efi_free_boot_services();
	}

	ftrace_init();

	/* Do the rest non-__init'ed, we're now alive */
	rest_init();
}

/* Call all constructor functions linked into the kernel. */
static void __init do_ctors(void)
{
#ifdef CONFIG_CONSTRUCTORS
	ctor_fn_t *fn = (ctor_fn_t *) __ctors_start;

	for (; fn < (ctor_fn_t *) __ctors_end; fn++)
		(*fn)();
#endif
}

bool initcall_debug;
core_param(initcall_debug, initcall_debug, bool, 0644);

#ifdef CONFIG_KALLSYMS
struct blacklist_entry {
	struct list_head next;
	char *buf;
};

static __initdata_or_module LIST_HEAD(blacklisted_initcalls);

static int __init initcall_blacklist(char *str)
{
	char *str_entry;
	struct blacklist_entry *entry;

	/* str argument is a comma-separated list of functions */
	do {
		str_entry = strsep(&str, ",");
		if (str_entry) {
			pr_debug("blacklisting initcall %s\n", str_entry);
			entry = alloc_bootmem(sizeof(*entry));
			entry->buf = alloc_bootmem(strlen(str_entry) + 1);
			strcpy(entry->buf, str_entry);
			list_add(&entry->next, &blacklisted_initcalls);
		}
	} while (str_entry);

	return 0;
}

static bool __init_or_module initcall_blacklisted(initcall_t fn)
{
	struct list_head *tmp;
	struct blacklist_entry *entry;
	char *fn_name;

	fn_name = kasprintf(GFP_KERNEL, "%pf", fn);
	if (!fn_name)
		return false;

	list_for_each(tmp, &blacklisted_initcalls) {
		entry = list_entry(tmp, struct blacklist_entry, next);
		if (!strcmp(fn_name, entry->buf)) {
			pr_debug("initcall %s blacklisted\n", fn_name);
			kfree(fn_name);
			return true;
		}
	}

	kfree(fn_name);
	return false;
}
#else
static int __init initcall_blacklist(char *str)
{
	pr_warn("initcall_blacklist requires CONFIG_KALLSYMS\n");
	return 0;
}

static bool __init_or_module initcall_blacklisted(initcall_t fn)
{
	return false;
}
#endif
__setup("initcall_blacklist=", initcall_blacklist);

static int __init_or_module do_one_initcall_debug(initcall_t fn)
{
	ktime_t calltime, delta, rettime;
	unsigned long long duration;
	int ret;

	printk(KERN_DEBUG "calling  %pF @ %i\n", fn, task_pid_nr(current));
	calltime = ktime_get();
	ret = fn();
	rettime = ktime_get();
	delta = ktime_sub(rettime, calltime);
	duration = (unsigned long long) ktime_to_ns(delta) >> 10;
	printk(KERN_DEBUG "initcall %pF returned %d after %lld usecs\n",
		 fn, ret, duration);

	return ret;
}

int __init_or_module do_one_initcall(initcall_t fn)
{
	int count = preempt_count();
	int ret;
	char msgbuf[64];

	if (initcall_blacklisted(fn))
		return -EPERM;

	if (initcall_debug)
		ret = do_one_initcall_debug(fn);
	else
		ret = fn();

	msgbuf[0] = 0;

	if (preempt_count() != count) {
		sprintf(msgbuf, "preemption imbalance ");
		preempt_count_set(count);
	}
	if (irqs_disabled()) {
		strlcat(msgbuf, "disabled interrupts ", sizeof(msgbuf));
		local_irq_enable();
	}
	WARN(msgbuf[0], "initcall %pF returned with %s\n", fn, msgbuf);

	return ret;
}


extern initcall_t __initcall_start[];
extern initcall_t __initcall0_start[];
extern initcall_t __initcall1_start[];
extern initcall_t __initcall2_start[];
extern initcall_t __initcall3_start[];
extern initcall_t __initcall4_start[];
extern initcall_t __initcall5_start[];
extern initcall_t __initcall6_start[];
extern initcall_t __initcall7_start[];
extern initcall_t __initcall_end[];

static initcall_t *initcall_levels[] __initdata = {
	__initcall0_start,
	__initcall1_start,
	__initcall2_start,
	__initcall3_start,
	__initcall4_start,
	__initcall5_start,
	__initcall6_start,
	__initcall7_start,
	__initcall_end,
};

/* Keep these in sync with initcalls in include/linux/init.h */
static char *initcall_level_names[] __initdata = {
	"early",
	"core",
	"postcore",
	"arch",
	"subsys",
	"fs",
	"device",
	"late",
};

static void __init do_initcall_level(int level)
{
	initcall_t *fn;

	strcpy(initcall_command_line, saved_command_line);
	parse_args(initcall_level_names[level],
		   initcall_command_line, __start___param,
		   __stop___param - __start___param,
		   level, level,
		   &repair_env_string);

	for (fn = initcall_levels[level]; fn < initcall_levels[level+1]; fn++)
		do_one_initcall(*fn);
}

static void __init do_initcalls(void)
{
	int level;

	for (level = 0; level < ARRAY_SIZE(initcall_levels) - 1; level++)
		do_initcall_level(level);
}

/**
 *	remove_channel_controls - removes produced/consumed control files
 */
static void remove_channel_controls(void)
{
	int i;
	
	for (i = 0; i < NR_CPUS; i++) {
		if (produced_control[i]) {
			debugfs_remove(produced_control[i]);
			produced_control[i] = NULL;
			continue;
		}
		break;
	}

	for (i = 0; i < NR_CPUS; i++) {
		if (consumed_control[i]) {
			debugfs_remove(consumed_control[i]);
			consumed_control[i] = NULL;
			continue;
		}
		break;
	}
}

/**
 *	create_channel_controls - creates produced/consumed control files
 *
 *	Returns channel on success, negative otherwise.
 */
static int create_channel_controls(struct dentry *parent,
				   const char *base_filename,
				   struct rchan *chan)
{
	unsigned int i;
	char *tmpname = kmalloc(NAME_MAX + 1, GFP_KERNEL);
	if (!tmpname)
		return -ENOMEM;

	for_each_online_cpu(i) {
		sprintf(tmpname, "%s%d.produced", base_filename, i);

		produced_control[i] = debugfs_create_file(tmpname, 0, parent, chan->buf[i], &produced_fops);
		if (!produced_control[i]) {
			printk("Couldn't create relayfs control file %s.\n",
			       tmpname);
			goto cleanup_control_files;
		}

		sprintf(tmpname, "%s%d.consumed", base_filename, i);
		consumed_control[i] = debugfs_create_file(tmpname, 0, parent, chan->buf[i], &consumed_fops);
		if (!consumed_control[i]) {
			printk("Couldn't create relayfs control file %s.\n",
			       tmpname);
			goto cleanup_control_files;
		}
	}

	return 0;

cleanup_control_files:
	remove_channel_controls();
	return -ENOMEM;
}

/*
 * subbuf_start() relayfs callback.
 *
 * Defined so that we can 1) reserve padding counts in the sub-buffers, and
 * 2) keep a count of events dropped due to the buffer-full condition.
 */
static int subbuf_start_handler(struct rchan_buf *buf,
				void *subbuf,
				void *prev_subbuf,
				unsigned int prev_padding)
{
	if (prev_subbuf)
		*((unsigned *)prev_subbuf) = prev_padding;

	if (relay_buf_full(buf)) {
		if (!suspended) {
			suspended = 1;
			printk("cpu %d buffer full!!!\n", smp_processor_id());
		}
		dropped++;
		return 0;
	} else if (suspended) {
		suspended = 0;
		printk("cpu %d buffer no longer full.\n", smp_processor_id());
	}

	subbuf_start_reserve(buf, sizeof(unsigned int));

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

/*
 * relayfs callbacks
 */
static struct rchan_callbacks relayfs_callbacks =
{
	.subbuf_start = subbuf_start_handler,
	.create_buf_file = create_buf_file_handler,
	.remove_buf_file = remove_buf_file_handler,
};

/**
 *	create_channel - creates channel /debug/APP_DIR/cpuXXX
 *
 *	Creates channel along with associated produced/consumed control files
 *
 *	Returns channel on success, NULL otherwise
 */
static struct rchan *create_channel(unsigned subbuf_size,
				    unsigned n_subbufs)
{
	struct rchan *chan;

	chan = relay_open("cpu", dir, subbuf_size,
			  n_subbufs, &relayfs_callbacks, NULL);
	
	if (!chan) {
		printk("relay app channel creation failed\n");
		return NULL;
	}

	if (create_channel_controls(dir, "cpu", chan)) {
		relay_close(chan);
		return NULL;
	}
	
	logging = 0;
	mappings = 0;
	suspended = 0;
	dropped = 0;

	return chan;
}

/**
 *	destroy_channel - destroys channel /debug/APP_DIR/cpuXXX
 *
 *	Destroys channel along with associated produced/consumed control files
 */
static void destroy_channel(void)
{
	if (chan) {
		relay_close(chan);
		chan = NULL;
	}
	remove_channel_controls();
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
		printk("Couldn't create relayfs control file 'enabled'.\n");
		goto fail;
	}
	
	subbuf_size_control = debugfs_create_file("subbuf_size", 0, dir,
						  NULL, &subbuf_size_fops);
	if (!subbuf_size_control) {
		printk("Couldn't create relayfs control file 'subbuf_size'.\n");
		goto fail;
	}

	n_subbufs_control = debugfs_create_file("n_subbufs", 0, dir,
						NULL, &n_subbufs_fops);
	if (!n_subbufs_control) {
		printk("Couldn't create relayfs control file 'n_subbufs'.\n");
		goto fail;
	}

	create_control = debugfs_create_file("create", 0, dir,
					     NULL, &create_fops);
	if (!create_control) {
		printk("Couldn't create relayfs control file 'create'.\n");
		goto fail;
	}

	dropped_control = debugfs_create_file("dropped", 0, dir,
					      NULL, &dropped_fops);
	if (!dropped_control) {
		printk("Couldn't create relayfs control file 'dropped'.\n");
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
 * control files for relayfs channel management
 */

static ssize_t enabled_read(struct file *filp, char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[64];
	
	snprintf(buf, sizeof(buf), "%d\n", logging);
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t enabled_write(struct file *filp, const char __user *buffer,
			     size_t count, loff_t *ppos)
{
	char buf[64];
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

	if (enabled && chan)
		logging = 1;
	else if (!enabled) {
		logging = 0;
		if (chan)
			relay_flush(chan);
	}
		
	return count;
}

/*
 * 'enabled' file operations - boolean r/w
 *
 *  toggles logging to the relayfs channel
 */
struct file_operations enabled_fops = {
	.read =		enabled_read,
	.write =	enabled_write,
};

static ssize_t create_read(struct file *filp, char __user *buffer,
			   size_t count, loff_t *ppos)
{
	char buf[64];
	
	snprintf(buf, sizeof(buf), "%d\n", !!chan);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t create_write(struct file *filp, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[64];
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
 *  creates/destroys the relayfs channel
 */
struct file_operations create_fops = {
	.read =		create_read,
	.write =	create_write,
};

static ssize_t subbuf_size_read(struct file *filp, char __user *buffer,
				size_t count, loff_t *ppos)
{
	char buf[64];
	
	snprintf(buf, sizeof(buf), "%zu\n", subbuf_size);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t subbuf_size_write(struct file *filp, const char __user *buffer,
				 size_t count, loff_t *ppos)
{
	char buf[64];
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
	.read =		subbuf_size_read,
	.write =	subbuf_size_write,
};

static ssize_t n_subbufs_read(struct file *filp, char __user *buffer,
			      size_t count, loff_t *ppos)
{
	char buf[64];
	
	snprintf(buf, sizeof(buf), "%zu\n", n_subbufs);
	
	return simple_read_from_buffer(buffer, count, ppos,
				       buf, strlen(buf));
}

static ssize_t n_subbufs_write(struct file *filp, const char __user *buffer,
			       size_t count, loff_t *ppos)
{
	char buf[64];
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
	.read =		n_subbufs_read,
	.write =	n_subbufs_write,
};

static ssize_t dropped_read(struct file *filp, char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char buf[64];
	
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
	.read =		dropped_read,
};


/*
 * control files for relayfs produced/consumed sub-buffer counts
 */

static int produced_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	
	return 0;
}

static ssize_t produced_read(struct file *filp, char __user *buffer,
			     size_t count, loff_t *ppos)
{
	struct rchan_buf *buf = filp->private_data;

	return simple_read_from_buffer(buffer, count, ppos,
				       &buf->subbufs_produced,
				       sizeof(buf->subbufs_produced));
}

/*
 * 'produced' file operations - r, binary
 *
 *  There is a .produced file associated with each per-cpu relayfs file.
 *  Reading a .produced file returns the number of sub-buffers so far
 *  produced for the associated relayfs buffer.
 */
struct file_operations produced_fops = {
	.open =		produced_open,
	.read =		produced_read
};

static int consumed_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	
	return 0;
}

static ssize_t consumed_read(struct file *filp, char __user *buffer,
			     size_t count, loff_t *ppos)
{
	struct rchan_buf *buf = filp->private_data;

	return simple_read_from_buffer(buffer, count, ppos,
				       &buf->subbufs_consumed,
				       sizeof(buf->subbufs_consumed));
}

static ssize_t consumed_write(struct file *filp, const char __user *buffer,
			      size_t count, loff_t *ppos)
{
	struct rchan_buf *buf = filp->private_data;
	size_t consumed;

	if (copy_from_user(&consumed, buffer, sizeof(consumed)))
		return -EFAULT;
		
	relay_subbufs_consumed(buf->chan, buf->cpu, consumed);

	return count;
}

/*
 * 'consumed' file operations - r/w, binary
 *
 *  There is a .consumed file associated with each per-cpu relayfs file.
 *  Writing to a .consumed file adds the value written to the
 *  subbuffers-consumed count of the associated relayfs buffer.
 *  Reading a .consumed file returns the number of sub-buffers so far
 *  consumed for the associated relayfs buffer.
 */
struct file_operations consumed_fops = {
	.open =		consumed_open,
	.read =		consumed_read,
	.write =	consumed_write,
};

/*
 * Ok, the machine is now initialized. None of the devices
 * have been touched yet, but the CPU subsystem is up and
 * running, and memory and process management works.
 *
 * Now we can finally start doing some real work..
 */
static void __init do_basic_setup(void)
{
	cpuset_init_smp();
	usermodehelper_init();
	shmem_init();
	driver_init();
	init_irq_proc();
	do_ctors();
	usermodehelper_enable();
	do_initcalls();
	
	// debugfs init
	dir = debugfs_create_dir(APP_DIR, NULL);
	if (!dir) {
		printk("Couldn't create debugfs app directory.\n");
	}

	if (create_controls()) {
		debugfs_remove(dir);
	}
		
	random_int_secret_init();
}

static void __init do_pre_smp_initcalls(void)
{
	initcall_t *fn;

	for (fn = __initcall_start; fn < __initcall0_start; fn++)
		do_one_initcall(*fn);
}

/*
 * This function requests modules which should be loaded by default and is
 * called twice right after initrd is mounted and right before init is
 * exec'd.  If such modules are on either initrd or rootfs, they will be
 * loaded before control is passed to userland.
 */
void __init load_default_modules(void)
{
	load_default_elevator_module();
}

static int run_init_process(const char *init_filename)
{
	argv_init[0] = init_filename;
	return do_execve(getname_kernel(init_filename),
		(const char __user *const __user *)argv_init,
		(const char __user *const __user *)envp_init);
}

static int try_to_run_init_process(const char *init_filename)
{
	int ret;

	ret = run_init_process(init_filename);

	if (ret && ret != -ENOENT) {
		pr_err("Starting init: %s exists but couldn't execute it (error %d)\n",
		       init_filename, ret);
	}

	return ret;
}

static noinline void __init kernel_init_freeable(void);

static int __ref kernel_init(void *unused)
{
	int ret;

	kernel_init_freeable();
	/* need to finish all async __init code before freeing the memory */
	async_synchronize_full();
	free_initmem();
	mark_rodata_ro();
	system_state = SYSTEM_RUNNING;
	numa_default_policy();

	flush_delayed_fput();

	if (ramdisk_execute_command) {
		ret = run_init_process(ramdisk_execute_command);
		if (!ret)
			return 0;
		pr_err("Failed to execute %s (error %d)\n",
		       ramdisk_execute_command, ret);
	}

	/*
	 * We try each of these until one succeeds.
	 *
	 * The Bourne shell can be used instead of init if we are
	 * trying to recover a really broken machine.
	 */
	if (execute_command) {
		ret = run_init_process(execute_command);
		if (!ret)
			return 0;
#ifndef CONFIG_INIT_FALLBACK
		panic("Requested init %s failed (error %d).",
		      execute_command, ret);
#else
		pr_err("Failed to execute %s (error %d).  Attempting defaults...\n",
		       execute_command, ret);
#endif
	}
	if (!try_to_run_init_process("/sbin/init") ||
	    !try_to_run_init_process("/etc/init") ||
	    !try_to_run_init_process("/bin/init") ||
	    !try_to_run_init_process("/bin/sh"))
		return 0;

	panic("No working init found.  Try passing init= option to kernel. "
	      "See Linux Documentation/init.txt for guidance.");
}

static noinline void __init kernel_init_freeable(void)
{
	/*
	 * Wait until kthreadd is all set-up.
	 */
	wait_for_completion(&kthreadd_done);

	/* Now the scheduler is fully set up and can do blocking allocations */
	gfp_allowed_mask = __GFP_BITS_MASK;

	/*
	 * init can allocate pages on any node
	 */
	set_mems_allowed(node_states[N_MEMORY]);
	/*
	 * init can run on any cpu.
	 */
	set_cpus_allowed_ptr(current, cpu_all_mask);

	cad_pid = task_pid(current);

	smp_prepare_cpus(setup_max_cpus);

	do_pre_smp_initcalls();
	lockup_detector_init();

	smp_init();
	sched_init_smp();

	do_basic_setup();

	/* Open the /dev/console on the rootfs, this should never fail */
	if (sys_open((const char __user *) "/dev/console", O_RDWR, 0) < 0)
		pr_err("Warning: unable to open an initial console.\n");

	(void) sys_dup(0);
	(void) sys_dup(0);
	/*
	 * check if there is an early userspace init.  If yes, let it do all
	 * the work
	 */

	if (!ramdisk_execute_command)
		ramdisk_execute_command = "/init";

	if (sys_access((const char __user *) ramdisk_execute_command, 0) != 0) {
		ramdisk_execute_command = NULL;
		prepare_namespace();
	}

	/*
	 * Ok, we have completed the initial bootup, and
	 * we're essentially up and running. Get rid of the
	 * initmem segments and start the user-mode stuff..
	 *
	 * rootfs is available now, try loading the public keys
	 * and default modules
	 */

	integrity_load_keys();
	load_default_modules();
}
