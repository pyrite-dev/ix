#include <sys/device.h>
#include <sys/disk.h>
#include <sys/diskdev.h>

#include "scsi.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SD_SECTOR_SIZE 512
#define SD_CD_BLOCK_SIZE 2048
#define SD_CD_SECTORS (SD_CD_BLOCK_SIZE / SD_SECTOR_SIZE)

struct sd_softc {
	struct disk disk;

	struct device* bus;
	unsigned int   target;
	unsigned int   lun;

	unsigned int nsectors;
	int	     cdrom;

	unsigned char cache[SD_CD_BLOCK_SIZE];
	unsigned int  cache_lba;
	int	      cache_valid;
};

static int sd_rw10(struct sd_softc* softc, unsigned int lba, void* data, unsigned int bytes, int read) {
	unsigned char cdb[10];
	int	      status;

	cdb[0] = read ? SCSI_OP_READ10 : SCSI_OP_WRITE10;
	cdb[1] = (softc->lun & 7) << 5;
	cdb[2] = (lba >> 24) & 0xff;
	cdb[3] = (lba >> 16) & 0xff;
	cdb[4] = (lba >> 8) & 0xff;
	cdb[5] = (lba >> 0) & 0xff;
	cdb[6] = 0;
	cdb[7] = 0;
	cdb[8] = 1;
	cdb[9] = 0;

	status = scsibus_exec(softc->bus, softc->target, softc->lun, cdb, sizeof(cdb), data, bytes, read);

	return status == SCSI_STATUS_GOOD ? 0 : -1;
}

static int sd_transfer(struct disk* disk, unsigned int sector, void* data, int read) {
	struct sd_softc* softc = disk->device->softc;

	if(sector >= softc->nsectors) return -1;

	if(softc->cdrom) {
		unsigned int lba = sector / SD_CD_SECTORS;
		unsigned int off = (sector % SD_CD_SECTORS) * SD_SECTOR_SIZE;

		if(!read) return -1;

		if(!softc->cache_valid || softc->cache_lba != lba) {
			if(sd_rw10(softc, lba, softc->cache, SD_CD_BLOCK_SIZE, 1) != 0) return -1;

			softc->cache_lba   = lba;
			softc->cache_valid = 1;
		}

		memcpy(data, softc->cache + off, SD_SECTOR_SIZE);

		return 0;
	}

	return sd_rw10(softc, sector, data, SD_SECTOR_SIZE, read);
}

static const struct disk_ops sd_disk_ops = {
    sd_transfer};

static char* sd_copy_field(char* dst, const unsigned char* src, int len) {
	char* start = dst;
	int   i;

	for(i = 0; i < len && src[i] != 0; i++) *dst++ = src[i];

	while(dst > start && dst[-1] == ' ') dst--;

	*dst = 0;

	return dst;
}

static int sd_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int sd_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct sd_softc* softc;
	unsigned char	 inquiry[36];
	unsigned char	 capacity[8];
	unsigned char	 cdb[10];
	unsigned int	 blocksize;
	unsigned int	 type;
	const char*	 devname = NULL;
	char		 name[8];
	char*		 p;
	int		 status;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	softc->bus    = parent;
	softc->target = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_TARGET], 0);
	softc->lun    = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_LUN], 0);

	cdb[0] = SCSI_OP_INQUIRY;
	cdb[1] = (softc->lun & 7) << 5;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = sizeof(inquiry);
	cdb[5] = 0;

	status = scsibus_exec(softc->bus, softc->target, softc->lun, cdb, 6, inquiry, sizeof(inquiry), 1);
	if(status != SCSI_STATUS_GOOD) goto fail;

	type = inquiry[0] & 0x1f;
	if(type != 0 && type != 5) goto fail;

	memset(cdb, 0, sizeof(cdb));
	cdb[0] = SCSI_OP_READ_CAPACITY;
	cdb[1] = (softc->lun & 7) << 5;

	status = scsibus_exec(softc->bus, softc->target, softc->lun, cdb, sizeof(cdb), capacity, sizeof(capacity), 1);
	if(status != SCSI_STATUS_GOOD) goto fail;

	softc->nsectors = (((unsigned int)capacity[0] << 24) | ((unsigned int)capacity[1] << 16) | ((unsigned int)capacity[2] << 8) | capacity[3]) + 1;
	blocksize	= ((unsigned int)capacity[4] << 24) | ((unsigned int)capacity[5] << 16) | ((unsigned int)capacity[6] << 8) | capacity[7];

	if(softc->nsectors == 0) goto fail;

	if(type == 0) {
		if(blocksize != SD_SECTOR_SIZE) goto fail;
	} else {
		if(blocksize != SD_CD_BLOCK_SIZE) goto fail;
		if(softc->nsectors > 0xffffffffU / SD_CD_SECTORS) goto fail;

		softc->nsectors *= SD_CD_SECTORS;
		softc->cdrom = 1;
	}

	p    = self->desc;
	*p++ = '<';
	p    = sd_copy_field(p, inquiry + 8, 8);
	if(p[-1] != '<') *p++ = ' ';
	p = sd_copy_field(p, inquiry + 16, 16);
	if(p[-1] == ' ') p--;
	*p++ = '>';
	*p   = 0;

	if(softc->cdrom) {
		snprintf(name, sizeof(name), "cd%u", self->unit);
		devname = name;
	}

	if(diskdev_init(self, devname, &sd_disk_ops, softc->nsectors) == DEVICE_ERROR) goto fail;

	if(softc->cdrom) {
		struct disk* part;

		softc->disk.readonly = 1;

		for(part = softc->disk.partitions; part != NULL; part = part->next) {
			part->readonly = 1;
		}
	}

	return 0;

fail:
	free(softc);
	self->softc = NULL;
	return -1;
}

static void sd_detach(struct device* self) {
	struct sd_softc* softc = self->softc;

	if(softc == NULL) return;

	diskdev_detach(&softc->disk);

	free(softc);
	self->softc = NULL;
}

struct device_driver sd_driver = {
    "sd",
    sd_match,
    sd_attach,
    NULL,
    sd_detach,
    NULL};
