#ifndef __SYS_DISK_H__
#define __SYS_DISK_H__

struct disk_ops;
struct disk;

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/buf.h>

struct disk_ops {
	int (*transfer)(struct disk* disk, unsigned int sector, void* data, int read);
};

struct disk {
	struct device* device;
	dev_t	       dev;

	const struct disk_ops* ops;

	unsigned int nsectors; /* capacity in 512-byte sectors */
	mutex_t	     lock;
};

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
int  disk_open(struct disk* disk, int flags, int mode);
int  disk_close(struct disk* disk, int flags, int mode);
void disk_strategy(struct disk* disk, struct buf* buf);
int  disk_ioctl(struct disk* disk, unsigned long cmd, void* data, int flags);
#endif

__END_EXTERN_C__

#endif
