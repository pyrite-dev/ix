#ifndef __SYS_TTYDEV_H__
#define __SYS_TTYDEV_H__

struct ttydev_ops;

#include <sys/tty.h>
#include <sys/termios.h>

struct ttydev_ops {
	void (*start)(struct tty* tty);
	int (*param)(struct tty* tty, const struct termios* termios);
	void (*stop)(struct tty* tty, int flags);
};

#endif
