#include <sys/disk.h>
#include <sys/bdev.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/major.h>

#include <stdlib.h>

#include <dyntypes.h>

struct view_map {
	dev_t	     key;
	struct disk* value;
};

static struct view_map* view_map;

static struct disk* diskdev_lookup(dev_t dev) {
	struct disk*   view;
	struct device* device;

	view = hmget(view_map, dev);
	if(view != NULL) return view;

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

dev_t diskdev_partition_init(struct disk* disk, const char* name, unsigned int start, unsigned int nsectors) {
	struct disk* view;
	dev_t	     dev = DEVICE_ERROR;
	char	     devname[33];
	char	     alpha;

	if(nsectors == 0 || start > disk->nsectors || nsectors > disk->nsectors - start) return DEVICE_ERROR;

	view = malloc(sizeof(*view));
	if(view == NULL) return DEVICE_ERROR;

	disk_init(view, disk->ops);

	view->device   = disk->device;
	view->parent   = disk;
	view->start    = start;
	view->nsectors = nsectors;

	mutex_init(&view->lock);

	if(name == NULL) {
		for(alpha = 'a'; alpha <= 'z'; alpha++) {
			dev_t d;

			snprintf(devname, sizeof(devname), "%s%d%c", disk->device->name, disk->device->unit, alpha);

			if(bdev_find_device(devname, &d) == 0) continue;

			break;
		}

		if(alpha > 'z') {
			free(view);
			return DEVICE_ERROR;
		}
	} else {
		snprintf(devname, sizeof(devname), "%s", name);
	}

	if((dev = bdev_register_device(DEVICE_MAKE(BDEV_MAJOR_DISK, DEVICE_MINOR_ANY), disk->device, devname)) == DEVICE_ERROR) {
		free(view);
		return DEVICE_ERROR;
	}

	view->dev = dev;

	hmput(view_map, dev, view);

	return dev;
}

void diskdev_partition_detach(dev_t dev) {
	struct disk* view;

	view = hmget(view_map, dev);
	if(view == NULL) return;

	hmdel(view_map, dev);

	bdev_unregister_device(dev);

	free(view);
}

void diskdev_partition_reset(struct device* device) {
	struct disk* disk = device->softc;
	int	     i;

	for(i = 0; i < hmlen(view_map); i++) {
		if(view_map[i].value->device == device) {
			diskdev_partition_detach(view_map[i].key);
			i--;
		}
	}
}

void diskdev_detach(struct device* device) {
	struct disk* disk = device->softc;

	diskdev_partition_reset(device);

	bdev_unregister_device(disk->dev);
}
