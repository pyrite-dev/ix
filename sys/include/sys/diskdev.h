#ifndef __SYS_DISKDEV_H__
#define __SYS_DISKDEV_H__

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/bdev.h>
#include <sys/disk.h>

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
extern struct bdevsw disk_bdevsw;

dev_t diskdev_init(struct device* device, const char* name, const struct disk_ops* ops, unsigned int nsectors);
dev_t diskdev_partition_init(struct disk* disk, const char* name, unsigned int start, unsigned int nsectors);
void  diskdev_partition_detach(dev_t dev);
void  diskdev_partition_reset(struct disk* disk);
void  diskdev_detach(struct disk* disk);

int diskdev_try_mbr(dev_t dev);
#endif

__END_EXTERN_C__

#endif
