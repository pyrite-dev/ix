#include <sys/device.h>
#include <sys/cdev.h>
#include <sys/bdev.h>
#include <sys/buf.h>
#include <sys/major.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <sys/mutex.h>

#include "scsi.h"

#include <stdlib.h>
#include <string.h>

#define CD_BLOCK_SIZE 2048
#define CD_UNITS_PER_BLOCK (CD_BLOCK_SIZE / 512)

struct cd_softc {
	struct device*	       bus;
	struct scsi_transport* transport;

	unsigned int target;
	unsigned int lun;

	unsigned int nblocks;
	dev_t	     cdev;
	dev_t	     bdev;

	unsigned char buffer[CD_BLOCK_SIZE];
	unsigned int  cached;

	mutex_t lock;
};

static int cd_exec(struct cd_softc* softc, const unsigned char* cdb, unsigned int cdb_len, unsigned char* data, unsigned int data_len, int data_read) {
	return softc->transport->exec(softc->bus, softc->target, softc->lun, cdb, cdb_len, data, data_len, data_read);
}

static int cd_read_block(struct cd_softc* softc, unsigned int block) {
	unsigned char cdb[10];

	if(softc->cached == block) return 0;

	memset(cdb, 0, sizeof(cdb));

	cdb[0] = SCSI_OP_READ10;
	cdb[1] = (softc->lun & 7) << 5;
	cdb[2] = (block >> 24) & 0xff;
	cdb[3] = (block >> 16) & 0xff;
	cdb[4] = (block >> 8) & 0xff;
	cdb[5] = block & 0xff;
	cdb[8] = 1;

	if(cd_exec(softc, cdb, sizeof(cdb), softc->buffer, CD_BLOCK_SIZE, 1) != SCSI_STATUS_GOOD) return -1;

	softc->cached = block;

	return 0;
}

static char* cd_copy_field(char* dst, const unsigned char* src, int len) {
	char* start = dst;
	int   i;

	for(i = 0; i < len && src[i] != 0; i++) *dst++ = src[i];

	while(dst > start && dst[-1] == ' ') dst--;

	*dst = 0;

	return dst;
}

static int cd_open(dev_t dev, int flags, int mode) {
	if(cdev_lookup_device(dev) == NULL) return ENXIO;

	return 0;
}

static int cd_close(dev_t dev, int flags, int mode) {
	return 0;
}

static int cd_read(dev_t dev, struct uio* uio, int flags) {
	struct device*	 device;
	struct cd_softc* softc;
	int		 error = 0;

	device = cdev_lookup_device(dev);
	if(device == NULL) return ENXIO;

	softc = device->softc;

	mutex_lock(&softc->lock);

	while(uio->uio_resid > 0) {
		unsigned int block = (unsigned int)(uio->uio_offset / CD_BLOCK_SIZE);
		unsigned int off   = (unsigned int)(uio->uio_offset % CD_BLOCK_SIZE);
		size_t	     n;

		if(block >= softc->nblocks) break;

		if(cd_read_block(softc, block) != 0) {
			error = EIO;
			break;
		}

		n = CD_BLOCK_SIZE - off;
		if(n > uio->uio_resid) n = uio->uio_resid;

		error = uiomove(softc->buffer + off, n, uio);
		if(error != 0) break;
	}

	mutex_unlock(&softc->lock);

	return error;
}

static int cd_write(dev_t dev, struct uio* uio, int flags) {
	return EROFS;
}

static int cd_ioctl(dev_t dev, unsigned long cmd, void* data, int flags) {
	return ENOTTY;
}

struct cdevsw cd_cdevsw = {
    cd_open,
    cd_close,
    cd_read,
    cd_write,
    cd_ioctl};
static int cd_cdev_registered = 0;

static int cd_bdev_open(dev_t dev, int flags, int mode) {
	if(bdev_lookup_device(dev) == NULL) return ENXIO;

	return 0;
}

static int cd_bdev_close(dev_t dev, int flags, int mode) {
	return 0;
}

static void cd_bdev_strategy(struct buf* buf) {
	struct device*	 device;
	struct cd_softc* softc;
	unsigned int	 nunits;
	unsigned int	 i;

	buf->error = 0;
	buf->resid = buf->bcount;
	buf->flags &= ~(BUF_DONE | BUF_ERROR);

	device = bdev_lookup_device(buf->dev);
	if(device == NULL) {
		buf->error = ENXIO;
		buf->flags |= BUF_ERROR | BUF_DONE;
		return;
	}

	softc = device->softc;

	mutex_lock(&softc->lock);

	if(buf->bcount == 0) {
		buf->flags |= BUF_DONE;
		mutex_unlock(&softc->lock);
		return;
	}

	if(!(buf->flags & BUF_READ)) {
		buf->error = EROFS;
		buf->flags |= BUF_ERROR | BUF_DONE;
		mutex_unlock(&softc->lock);
		return;
	}

	nunits = softc->nblocks * CD_UNITS_PER_BLOCK;

	if(buf->bcount % 512 != 0 || buf->blkno >= nunits || buf->bcount / 512 > nunits - buf->blkno) {
		buf->error = EINVAL;
		buf->flags |= BUF_ERROR | BUF_DONE;
		mutex_unlock(&softc->lock);
		return;
	}

	for(i = 0; i < buf->bcount / 512; i++) {
		unsigned int unit  = buf->blkno + i;
		unsigned int block = unit / CD_UNITS_PER_BLOCK;
		unsigned int off   = (unit % CD_UNITS_PER_BLOCK) * 512;

		if(cd_read_block(softc, block) != 0) {
			buf->error = EIO;
			buf->flags |= BUF_ERROR;
			break;
		}

		memcpy((unsigned char*)buf->data + i * 512, softc->buffer + off, 512);

		buf->resid -= 512;
	}

	buf->flags |= BUF_DONE;

	mutex_unlock(&softc->lock);
}

static int cd_bdev_ioctl(dev_t dev, unsigned long cmd, void* data, int flags) {
	return ENOTTY;
}

static struct bdevsw cd_bdevsw = {
    cd_bdev_open,
    cd_bdev_close,
    cd_bdev_strategy,
    cd_bdev_ioctl};
static int cd_bdev_registered = 0;

static int cd_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int cd_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct cd_softc* softc;
	unsigned char	 inquiry[36];
	unsigned char	 capacity[8];
	unsigned char	 cdb[10];
	unsigned int	 blocksize;
	char*		 p;
	int		 status;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	softc->bus	 = parent;
	softc->transport = (struct scsi_transport*)parent->softc;
	softc->target	 = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_TARGET], 0);
	softc->lun	 = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_LUN], 0);
	softc->cached	 = (unsigned int)-1;

	mutex_init(&softc->lock);

	memset(cdb, 0, sizeof(cdb));

	cdb[0] = SCSI_OP_INQUIRY;
	cdb[1] = (softc->lun & 7) << 5;
	cdb[4] = sizeof(inquiry);

	status = cd_exec(softc, cdb, 6, inquiry, sizeof(inquiry), 1);
	if(status != SCSI_STATUS_GOOD) {
		free(softc);
		return -1;
	}

	if((inquiry[0] & 0x1f) != 5) {
		free(softc);
		return -1;
	}

	memset(cdb, 0, sizeof(cdb));

	cdb[0] = SCSI_OP_READ_CAPACITY;
	cdb[1] = (softc->lun & 7) << 5;

	status = cd_exec(softc, cdb, sizeof(cdb), capacity, sizeof(capacity), 1);
	if(status != SCSI_STATUS_GOOD) {
		free(softc);
		return -1;
	}

	softc->nblocks = (((unsigned int)capacity[0] << 24) | ((unsigned int)capacity[1] << 16) | ((unsigned int)capacity[2] << 8) | capacity[3]) + 1;
	blocksize      = ((unsigned int)capacity[4] << 24) | ((unsigned int)capacity[5] << 16) | ((unsigned int)capacity[6] << 8) | capacity[7];

	if(softc->nblocks == 0 || blocksize != CD_BLOCK_SIZE) {
		free(softc);
		return -1;
	}

	p    = self->desc;
	*p++ = '<';
	p    = cd_copy_field(p, inquiry + 8, 8);
	if(p[-1] != '<') *p++ = ' ';
	p = cd_copy_field(p, inquiry + 16, 16);
	if(p[-1] == ' ') p--;
	*p++ = '>';
	*p   = 0;

	if(!cd_cdev_registered) {
		if(cdev_register(CDEV_MAJOR_CDROM, &cd_cdevsw) != 0) {
			free(softc);
			return -1;
		}

		cd_cdev_registered = 1;
	}

	if(!cd_bdev_registered) {
		if(bdev_register(BDEV_MAJOR_CDROM, &cd_bdevsw) != 0) {
			free(softc);
			return -1;
		}

		cd_bdev_registered = 1;
	}

	if((softc->cdev = cdev_register_device(DEVICE_MAKE(CDEV_MAJOR_CDROM, DEVICE_MINOR_ANY), self, "rcd%d")) == DEVICE_ERROR) {
		free(softc);
		return -1;
	}

	if((softc->bdev = bdev_register_device(DEVICE_MAKE(BDEV_MAJOR_CDROM, DEVICE_MINOR_ANY), self, NULL)) == DEVICE_ERROR) {
		cdev_unregister_device(softc->cdev);
		free(softc);
		return -1;
	}

	return 0;
}

static void cd_detach(struct device* self) {
	struct cd_softc* softc = self->softc;

	bdev_unregister_device(softc->bdev);
	cdev_unregister_device(softc->cdev);

	free(softc);
}

struct device_driver cd_driver = {
    "cd",
    cd_match,
    cd_attach,
    NULL,
    cd_detach,
    NULL};
