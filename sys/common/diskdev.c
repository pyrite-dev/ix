#include <sys/disk.h>
#include <sys/bdev.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/major.h>

static struct disk* diskdev_lookup(dev_t dev) {
	struct device* device;

	device = bdev_lookup_device(dev);
	if(device == NULL) return NULL;

	return (struct disk*)device->softc;
}

static int diskdev_open(dev_t dev, int flags, int mode) {
	struct disk* disk;

	disk = diskdev_lookup(dev);
	if(disk == NULL) return ENXIO;

	return disk_open(disk, flags, mode);
}

static int diskdev_close(dev_t dev, int flags, int mode) {
	struct disk* disk;

	disk = diskdev_lookup(dev);
	if(disk == NULL) return ENXIO;

	return disk_close(disk, flags, mode);
}

static void diskdev_strategy(struct buf* buf) {
	struct disk* disk;

	buf->error = 0;
	buf->resid = buf->bcount;
	buf->flags &= ~(BUF_DONE | BUF_ERROR);

	disk = diskdev_lookup(buf->dev);
	if(disk == NULL) {
		buf->error = ENXIO;
		buf->flags |= BUF_ERROR | BUF_DONE;
		return;
	}

	return disk_strategy(disk, buf);
}

static int diskdev_ioctl(dev_t dev, unsigned long cmd, void* data, int flags) {
	struct disk* disk;

	disk = diskdev_lookup(dev);
	if(disk == NULL) return ENXIO;

	return disk_ioctl(disk, cmd, data, flags);
}

struct bdevsw disk_bdevsw = {
    diskdev_open,
    diskdev_close,
    diskdev_strategy,
    diskdev_ioctl};

dev_t diskdev_init(struct device* device, const char* name, const struct disk_ops* ops, unsigned int nsectors) {
	struct disk* disk = device->softc;

	disk_init(disk, ops);

	disk->device   = device;
	disk->nsectors = nsectors;

	mutex_init(&disk->lock);

	if((disk->dev = bdev_register_device(DEVICE_MAKE(BDEV_MAJOR_DISK, DEVICE_MINOR_ANY), device, name)) == DEVICE_ERROR) return DEVICE_ERROR;

	return disk->dev;
}

void diskdev_detach(struct device* device) {
	struct disk* disk = device->softc;

	bdev_unregister_device(disk->dev);
}
