#include <sys/cons.h>
#include <sys/machdep.h>
#include <sys/interrupt.h>
#include <sys/osinfo.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/syscall.h>
#include <sys/vfs.h>

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

static thread_t idle_thread;
static char	idle_stack[256];

static void idle_entry(void* arg) {
	/* TODO: Do not use busy loop here!!! */

	while(1);
}

static thread_t thread;
static char	stack[4096];

static void thread_main(void* arg) {
	const char* text = "Hello, Console!\n";
	int	    fd	 = open("/dev/tty0", O_RDWR);
	DIR*	    dir	 = opendir("/dev");

	write(fd, text, strlen(text));

	if(dir != NULL) {
		struct dirent* d;

		while((d = readdir(dir)) != NULL) {
			write(fd, d->d_name, strlen(d->d_name));
			write(fd, "\n", 1);
		}
	}

	while(1) {
		char c;

		read(fd, &c, 1);
		write(fd, &c, 1);
	}
}

void kmain(void) {
	intr_flags_t flags;

	machdep_init();
	cons_init();
	intr_init();
	syscall_init();

	kprintf("Marc/%s %s (%s): %s\r\n", os_machine, os_version, os_ident, os_date);
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

	vfs_init();
	file_init();

	sched_start();
}
