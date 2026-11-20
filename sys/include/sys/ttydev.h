#ifndef __SYS_TTYDEV_H__
#define __SYS_TTYDEV_H__

struct ttydev_ops;

#include <sys/cdefs.h>
#include <sys/tty.h>
#include <sys/termios.h>
#include <sys/device.h>

struct ttydev_ops {
	void (*start)(struct tty* tty);
	int (*param)(struct tty* tty, const struct termios* termios);
	void (*stop)(struct tty* tty, int flags);
};

__BEGIN_EXTERN_C__

int ttydev_init(void* softc, const struct ttydev_ops* ops, speed_t baud);
struct tty* ttydev_lookup(dev_t dev);

__END_EXTERN_C__

#endif
