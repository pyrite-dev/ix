#ifndef __SYS_TTYDEV_H__
#define __SYS_TTYDEV_H__

#include <sys/cdefs.h>
#include <sys/tty.h>
#include <sys/termios.h>
#include <sys/device.h>
#include <sys/cdev.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
extern struct cdevsw tty_cdevsw;

dev_t ttydev_init(struct device* device, const char* name, const struct tty_ops* ops, speed_t baud);
void  ttydev_detach(struct device* device);
#endif

__END_EXTERN_C__

#endif
