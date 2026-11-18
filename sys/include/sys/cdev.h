#ifndef __SYS_CONS_H__
#define __SYS_CONS_H__

struct cdevsw;

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/uio.h>

struct cdevsw {
	int (*open)(dev_t dev, int flags, int mode);
	int (*close)(dev_t dev, int flags, int mode);
	int (*read)(dev_t dev, struct uio* uio, int flags);
	int (*write)(dev_t dev, struct uio* uio, int flags);
	int (*ioctl)(dev_t dev, unsigned long cmd, void* data, int flags);
};

__BEGIN_EXTERN_C__

__END_EXTERN_C__

#endif
