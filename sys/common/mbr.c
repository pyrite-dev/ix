#include <sys/diskdev.h>
#include <sys/bootblock.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/bdev.h>
#include <sys/buf.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/endian.h>

#include <string.h>

int diskdev_try_mbr(dev_t dev) {
	struct device*	  device;
	struct disk*	  disk;
	struct mbr_sector sector;
	struct bdevsw*	  sw;
	struct buf	  b;
	int		  err;
	int		  i;

	if((device = bdev_lookup_device(dev)) == NULL) return -1;
	disk = device->softc;

	if((sw = bdev_lookup(DEVICE_MAJOR(dev))) == NULL) return -1;

	err = (sw->open == NULL) ? ENXIO : sw->open(dev, O_RDONLY, 0);
	if(err != 0) return -1;

	memset(&b, 0, sizeof(b));
	b.dev	 = dev;
	b.blkno	 = 0;
	b.data	 = &sector;
	b.bcount = sizeof(sector);
	b.flags	 = BUF_READ;

	sw->strategy(&b);

	if((b.flags & BUF_ERROR) || ltohs(sector.sig) != MBR_SIGNATURE) {
		if(sw->close != NULL) sw->close(dev, O_RDONLY, 0);
		return -1;
	}

	for(i = 0; i < 4; i++) {
		struct mbr_partition* part = &sector.table[i];

		if(part->type == 0) continue;

		diskdev_partition_init(disk, NULL, ltohl(part->start_lba), ltohl(part->sectors));
	}

	if(sw->close != NULL) sw->close(dev, O_RDONLY, 0);

	return 0;
}
