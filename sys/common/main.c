#include <sys/cons.h>
#include <sys/machdep.h>
#include <sys/interrupt.h>
#include <sys/osinfo.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/sched.h>
#include <sys/thread.h>

static thread_t idle_thread;
static char	idle_stack[256];

static void idle_entry(void* arg) {
	/* TODO: Do not use busy loop here!!! */

	while(1);
}

static thread_t thread1;
static char	stack1[2048];

static thread_t thread2;
static char	stack2[2048];

static spinlock_t lock;

static void test(void* arg) {
	while(1) {
		spinlock_lock(&lock);
		kprintf("%d", (int)arg);
		spinlock_unlock(&lock);
	}
}

void kmain(void) {
	machdep_init();
	cons_init();
	intr_init();

	kprintf("IX/%s %s (%s): %s\r\n", os_machine, os_version, os_ident, os_date);
	kprintf("avail mem = %ld KB\r\n", (long)kmem_avail / 1024);
	kprintf("\r\n");

	thread_init(&idle_thread, idle_stack, sizeof(idle_stack), idle_entry, NULL);

	sched_init(&idle_thread);
	device_init();

	spinlock_init(&lock);

	thread_init(&thread1, stack1, sizeof(stack1), test, (void*)1);
	thread_init(&thread2, stack2, sizeof(stack2), test, (void*)2);

	sched_add(&thread1);
	sched_add(&thread2);

	sched_start();
}
