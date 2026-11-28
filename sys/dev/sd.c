#include <sys/device.h>
#include <sys/disk.h>
#include <sys/diskdev.h>

#include "scsi.h"

#include <stdlib.h>
#include <string.h>

struct sd_softc {
	struct disk disk;

	struct device* bus;
	unsigned int   target;
	unsigned int   lun;

	unsigned int nsectors;
};

#define SD_MAX_SECTOR 0x200000

static int sd_transfer(struct disk* disk, unsigned int sector, void* data, int read) {
	struct sd_softc* softc = disk->device->softc;
	unsigned char	 cdb[6];
	int		 status;

	if(sector >= softc->nsectors || sector >= SD_MAX_SECTOR) return -1;

	cdb[0] = read ? SCSI_OP_READ6 : SCSI_OP_WRITE6;
	cdb[1] = ((softc->lun & 7) << 5) | ((sector >> 16) & 0x1f);
	cdb[2] = (sector >> 8) & 0xff;
	cdb[3] = (sector >> 0) & 0xff;
	cdb[4] = 1;
	cdb[5] = 0;

	status = scsibus_exec(softc->bus, softc->target, softc->lun, cdb, sizeof(cdb), data, 512, read);

	return status == SCSI_STATUS_GOOD ? 0 : -1;
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
	if(status != SCSI_STATUS_GOOD) {
		free(softc);
		return -1;
	}

	if((inquiry[0] & 0x1f) != 0) {
		free(softc);
		return -1;
	}

	memset(cdb, 0, sizeof(cdb));
	cdb[0] = SCSI_OP_READ_CAPACITY;
	cdb[1] = (softc->lun & 7) << 5;

	status = scsibus_exec(softc->bus, softc->target, softc->lun, cdb, sizeof(cdb), capacity, sizeof(capacity), 1);
	if(status != SCSI_STATUS_GOOD) {
		free(softc);
		return -1;
	}

	softc->nsectors = (((unsigned int)capacity[0] << 24) | ((unsigned int)capacity[1] << 16) | ((unsigned int)capacity[2] << 8) | capacity[3]) + 1;
	blocksize	= ((unsigned int)capacity[4] << 24) | ((unsigned int)capacity[5] << 16) | ((unsigned int)capacity[6] << 8) | capacity[7];

	if(softc->nsectors == 0 || blocksize != 512) {
		free(softc);
		return -1;
	}

	p = self->desc;
	*p++ = '<';
	p    = sd_copy_field(p, inquiry + 8, 8);
	if(p[-1] != '<') *p++ = ' ';
	p = sd_copy_field(p, inquiry + 16, 16);
	if(p[-1] == ' ') p--;
	*p++ = '>';
	*p   = 0;

	if(diskdev_init(self, NULL, &sd_disk_ops, softc->nsectors) == DEVICE_ERROR) {
		free(softc);
		return -1;
	}

	return 0;
}

static void sd_detach(struct device* self) {
	struct sd_softc* softc = self->softc;

	diskdev_detach(&softc->disk);

	free(softc);
}

struct device_driver sd_driver = {
    "sd",
    sd_match,
    sd_attach,
    NULL,
    sd_detach};
