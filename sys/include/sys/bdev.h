#ifndef __SYS_BDEV_H__
#define __SYS_BDEV_H__

struct bdevsw;

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/device.h>

struct bdevsw {
	int (*open)(dev_t dev, int flags, int mode);
	int (*close)(dev_t dev, int flags, int mode);
	void (*strategy)(struct buf* buf);
	int (*ioctl)(dev_t dev, unsigned long cmd, void* data, int flags);
};

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void bdev_init(void);

int  bdev_register(unsigned int major, struct bdevsw* bdevsw);
void bdev_unregister(unsigned int major);

struct bdevsw* bdev_lookup(unsigned int dev);

dev_t bdev_register_device(dev_t dev, struct device* device);
void  bdev_unregister_device(dev_t dev);

struct device* bdev_lookup_device(dev_t dev);
int	       bdev_find_device(const char* name, dev_t* result);
#endif

__END_EXTERN_C__

#endif
