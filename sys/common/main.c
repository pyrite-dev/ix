#include <sys/cons.h>
#include <sys/machdep.h>
#include <sys/interrupt.h>
#include <sys/osinfo.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/syscall.h>

#include <sys/bdev.h>
#include <sys/buf.h>
#include <sys/major.h>

#include <string.h>

static thread_t idle_thread;
static char	idle_stack[256];

static void idle_entry(void* arg) {
	/* TODO: Do not use busy loop here!!! */

	while(1);
}

static thread_t thread;
static char	stack[2048];

static void thread_main(void* arg) {
	struct bdevsw* sw  = bdev_lookup(BDEV_MAJOR_DISK);
	dev_t	       dev = DEVICE_MAKE(BDEV_MAJOR_DISK, 0);
	struct buf     b;
	unsigned char  data[512];
	int	       i;

	kprintf("syscall test: %s\r\n", strerror(_syscall_invoke(0, 0, 0, 0, 0, 0)));

	memset(&b, 0, sizeof(b));
	b.dev	 = dev;
	b.blkno	 = 0;
	b.data	 = data;
	b.bcount = sizeof(data);
	b.flags	 = BUF_READ;

	sw->strategy(&b);

	if(!(b.flags & BUF_ERROR)) {
		for(i = 0; i < 512; i++) {
			if(i > 0 && (i % 16) == 0) kprintf("\r\n");

			kprintf("%x ", data[i]);
		}
		kprintf("\r\n");
	}
}

void kmain(void) {
	intr_flags_t flags;

	machdep_init();
	cons_init();
	intr_init();
	syscall_init();

	kprintf("IX/%s %s (%s): %s\r\n", os_machine, os_version, os_ident, os_date);
	kprintf("Available memory = %ld KB\r\n", (long)kmem_avail / 1024);
	kprintf("\r\n");

	thread_init(&idle_thread, idle_stack, sizeof(idle_stack), idle_entry, NULL);

	sched_init(&idle_thread);
	device_init();

	thread_init(&thread, stack, sizeof(stack), thread_main, NULL);
	sched_add(&thread);

	flags = _intr_save();
	kmem_use_spinlock();
	device_start();
	_intr_restore(flags);

	sched_start();
}
