#ifndef __SYS_CDEV_H__
#define __SYS_CDEV_H__

struct cdevsw;

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/device.h>

struct cdevsw {
	int (*open)(dev_t dev, int flags, int mode);
	int (*close)(dev_t dev, int flags, int mode);
	int (*read)(dev_t dev, struct uio* uio, int flags);
	int (*write)(dev_t dev, struct uio* uio, int flags);
	int (*ioctl)(dev_t dev, unsigned long cmd, void* data, int flags);
};

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void cdev_init(void);

int  cdev_register(unsigned int major, struct cdevsw* cdevsw);
void cdev_unregister(unsigned int major);

struct cdevsw* cdev_lookup(unsigned int dev);

dev_t cdev_register_device(dev_t dev, struct device* device);
void  cdev_unregister_device(dev_t dev);

struct device* cdev_lookup_device(dev_t dev);
int	       cdev_find_device(const char* name, dev_t* result);
int	       cdev_first_device(unsigned int major, dev_t* result);
#endif

__END_EXTERN_C__

#endif
