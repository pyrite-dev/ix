#ifndef __SYS_TTYDEV_H__
#define __SYS_TTYDEV_H__

struct ttydev_ops;
struct device_tty;

#include <sys/cdefs.h>
#include <sys/tty.h>
#include <sys/termios.h>
#include <sys/device.h>
#include <sys/cdev.h>

struct ttydev_ops {
	void (*start)(struct tty* tty);
	int (*param)(struct tty* tty, const struct termios* termios);
	void (*stop)(struct tty* tty, int flags);
};

__BEGIN_EXTERN_C__

extern struct cdevsw tty_cdevsw;

int ttydev_init(void* softc, const struct ttydev_ops* ops, speed_t baud);

struct tty* ttydev_lookup(dev_t dev);

__END_EXTERN_C__

#endif
