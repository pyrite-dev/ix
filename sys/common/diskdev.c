#include <sys/diskdev.h>
#include <sys/disk.h>
#include <sys/bdev.h>
#include <sys/cdev.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <sys/major.h>

#include <stdlib.h>

static struct disk* diskdev_lookup2(dev_t dev, int raw) {
	struct disk*   disk;
	struct disk*   view;
	struct device* device;

	device = raw ? cdev_lookup_device(dev) : bdev_lookup_device(dev);
	if(device == NULL) return NULL;

	disk = (struct disk*)device->softc;

	if((raw ? disk->rdev : disk->dev) == dev) return disk;

	for(view = disk->partitions; view != NULL; view = view->next) {
		if((raw ? view->rdev : view->dev) == dev) return view;
	}

	return NULL;
}

static struct disk* diskdev_lookup(dev_t dev) {
	return diskdev_lookup2(dev, 0);
}

static struct disk* diskdev_lookup_raw(dev_t dev) {
	return diskdev_lookup2(dev, 1);
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

static int diskdev_raw_open(dev_t dev, int flags, int mode) {
	struct disk* disk;

	disk = diskdev_lookup_raw(dev);
	if(disk == NULL) return ENXIO;

	return disk_open(disk, flags, mode);
}

static int diskdev_raw_close(dev_t dev, int flags, int mode) {
	struct disk* disk;

	disk = diskdev_lookup_raw(dev);
	if(disk == NULL) return ENXIO;

	return disk_close(disk, flags, mode);
}

static int diskdev_raw_rw(dev_t dev, struct uio* uio, int read) {
	struct disk* disk;
	struct disk* phys;
	int	     error = 0;

	disk = diskdev_lookup_raw(dev);
	if(disk == NULL) return ENXIO;

	phys = disk->parent != NULL ? disk->parent : disk;

	mutex_lock(&phys->lock);

	while(uio->uio_resid > 0) {
		unsigned int sector = (unsigned int)(uio->uio_offset / 512);
		unsigned int off    = (unsigned int)(uio->uio_offset % 512);
		size_t	     n;

		if(sector >= disk->nsectors) break;

		n = 512 - off;
		if(n > uio->uio_resid) n = uio->uio_resid;

		if(read || off != 0 || n < 512) {
			if(phys->ops->transfer(phys, disk->start + sector, disk->bounce, 1) != 0) {
				error = EIO;
				break;
			}
		}

		error = uiomove(disk->bounce + off, n, uio);
		if(error != 0) break;

		if(!read) {
			if(phys->ops->transfer(phys, disk->start + sector, disk->bounce, 0) != 0) {
				error = EIO;
				break;
			}
		}
	}

	mutex_unlock(&phys->lock);

	return error;
}

static int diskdev_raw_read(dev_t dev, struct uio* uio, int flags) {
	return diskdev_raw_rw(dev, uio, 1);
}

static int diskdev_raw_write(dev_t dev, struct uio* uio, int flags) {
	return diskdev_raw_rw(dev, uio, 0);
}

static int diskdev_raw_ioctl(dev_t dev, unsigned long cmd, void* data, int flags) {
	struct disk* disk;

	disk = diskdev_lookup_raw(dev);
	if(disk == NULL) return ENXIO;

	return disk_ioctl(disk, cmd, data, flags);
}

struct cdevsw disk_cdevsw = {
    diskdev_raw_open,
    diskdev_raw_close,
    diskdev_raw_read,
    diskdev_raw_write,
    diskdev_raw_ioctl};

static dev_t diskdev_register_raw(struct device* device, struct disk* disk) {
	char rdevname[34];

	snprintf(rdevname, sizeof(rdevname), "r%s", bdev_device_name(disk->dev));

	return cdev_register_device(DEVICE_MAKE(CDEV_MAJOR_DISK, DEVICE_MINOR_ANY), device, rdevname);
}

dev_t diskdev_init(struct device* device, const char* name, const struct disk_ops* ops, unsigned int nsectors) {
	struct disk* disk = device->softc;

	disk_init(disk, ops);

	disk->device   = device;
	disk->nsectors = nsectors;

	mutex_init(&disk->lock);

	if((disk->dev = bdev_register_device(DEVICE_MAKE(BDEV_MAJOR_DISK, DEVICE_MINOR_ANY), device, name)) == DEVICE_ERROR) return DEVICE_ERROR;

	if((disk->rdev = diskdev_register_raw(device, disk)) == DEVICE_ERROR) {
		bdev_unregister_device(disk->dev);
		return DEVICE_ERROR;
	}

	diskdev_try_mbr(disk->dev);

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
		char* base = bdev_device_name(disk->dev);

		for(alpha = 'a'; alpha <= 'z'; alpha++) {
			dev_t d;

			snprintf(devname, sizeof(devname), "%s%c", base, alpha);

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

	if((view->rdev = diskdev_register_raw(disk->device, view)) == DEVICE_ERROR) {
		bdev_unregister_device(dev);
		free(view);
		return DEVICE_ERROR;
	}

	view->next	 = disk->partitions;
	disk->partitions = view;

	return dev;
}

void diskdev_partition_detach(dev_t dev) {
	struct device* device;
	struct disk*   disk;
	struct disk**  link;
	struct disk*   view;

	device = bdev_lookup_device(dev);
	if(device == NULL) return;

	disk = (struct disk*)device->softc;

	for(link = &disk->partitions; (view = *link) != NULL; link = &view->next) {
		if(view->dev == dev) {
			*link = view->next;

			cdev_unregister_device(view->rdev);
			bdev_unregister_device(dev);

			free(view);

			return;
		}
	}
}

void diskdev_partition_reset(struct disk* disk) {
	struct disk* view;
	struct disk* next;

	for(view = disk->partitions; view != NULL; view = next) {
		next = view->next;

		diskdev_partition_detach(view->dev);
	}
}

void diskdev_detach(struct disk* disk) {
	diskdev_partition_reset(disk);

	cdev_unregister_device(disk->rdev);
	bdev_unregister_device(disk->dev);
}
