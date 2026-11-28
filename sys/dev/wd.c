#include <sys/device.h>
#include <sys/bus.h>
#include <sys/cons.h>
#include <sys/time.h>
#include <sys/disk.h>
#include <sys/diskdev.h>
#include <sys/major.h>

#include "wd.h"
#include "wdc.h"

#include <stdlib.h>
#include <stddef.h>

struct wd_softc {
	struct disk disk;

	struct wdc_softc* wdc;
};

static int wait_drq(bus_space_t* space) {
	unsigned long limit = ticktime(100);
	unsigned long start = ticks;
	unsigned char status;

	while(1) {
		status = bus_space_read8(space, WD_SR);

		if(status & WD_S_BSY) {
			if(ticks - start >= limit) return -1;
			continue;
		}
		if(status & (WD_S_ERR | WD_S_DF)) return -1;
		if(status & WD_S_DRQ) return 0;
		if(ticks - start >= limit) return -1;
	}
}

static int wait_bsy(bus_space_t* space) {
	unsigned long limit = ticktime(100);
	unsigned long start = ticks;
	unsigned char status;

	while(1) {
		status = bus_space_read8(space, WD_SR);

		if(status & (WD_S_ERR | WD_S_DF)) return -1;
		if(!(status & WD_S_BSY)) return 0;
		if(ticks - start >= limit) return -1;
	}
}

static int wd_transfer(struct disk* disk, unsigned int sector, void* data_, int read) {
	struct wd_softc* softc = disk->device->softc;
	unsigned char*	 data  = data_;
	int		 i;

	spinlock_lock(&softc->wdc->lock);
	if(wait_bsy(&softc->wdc->space) != 0) {
		spinlock_unlock(&softc->wdc->lock);
		return -1;
	}

	bus_space_write8(&softc->wdc->space, WD_DHR, WD_DH_MASTER | WD_DH_ALWAYS | ((sector >> 24) & 0x0f));
	bus_space_write8(&softc->wdc->space, WD_SCR, 1);
	bus_space_write8(&softc->wdc->space, WD_LBALR, (sector >> 0) & 0xff);
	bus_space_write8(&softc->wdc->space, WD_LBAMR, (sector >> 8) & 0xff);
	bus_space_write8(&softc->wdc->space, WD_LBAHR, (sector >> 16) & 0xff);

	bus_space_write8(&softc->wdc->space, WD_CR, read ? WD_C_READ : WD_C_WRITE);

	if(wait_drq(&softc->wdc->space) != 0) {
		spinlock_unlock(&softc->wdc->lock);
		return -1;
	}

	if(read) {
		for(i = 0; i < 512; i += 2) {
			unsigned short n = bus_space_read16(&softc->wdc->space, WD_DR);

			data[i + 0] = (n >> 8) & 0xff;
			data[i + 1] = (n >> 0) & 0xff;
		}
		spinlock_unlock(&softc->wdc->lock);

		return 0;
	}

	for(i = 0; i < 512; i += 2) {
		unsigned short n = ((unsigned short)data[i + 0] << 8) | data[i + 1];

		bus_space_write16(&softc->wdc->space, WD_DR, n);
	}

	if(wait_bsy(&softc->wdc->space) != 0) {
		spinlock_unlock(&softc->wdc->lock);
		return -1;
	}
	spinlock_unlock(&softc->wdc->lock);

	return 0;
}

static const struct disk_ops wd_disk_ops = {
    wd_transfer};

static int wd_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int wd_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct wd_softc* softc;
	unsigned char	 status;
	unsigned char	 buffer[512];
	unsigned int	 nsectors;
	int		 i;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	softc->wdc = parent->softc;

	spinlock_lock(&softc->wdc->lock);
	bus_space_write8(&softc->wdc->space, WD_DHR, (args->locators[DEVICE_LOCATOR_DRIVE] ? WD_DH_SLAVE : WD_DH_MASTER) | WD_DH_ALWAYS);

	bus_space_write8(&softc->wdc->space, WD_SCR, 0);
	bus_space_write8(&softc->wdc->space, WD_LBALR, 0);
	bus_space_write8(&softc->wdc->space, WD_LBAMR, 0);
	bus_space_write8(&softc->wdc->space, WD_LBAHR, 0);

	bus_space_write8(&softc->wdc->space, WD_CR, WD_C_IDENTIFY);

	status = bus_space_read8(&softc->wdc->space, WD_SR);
	if(status == 0 || status == 0xff) {
		spinlock_unlock(&softc->wdc->lock);
		free(softc);
		return -1;
	}

	if(wait_drq(&softc->wdc->space) != 0) {
		spinlock_unlock(&softc->wdc->lock);
		free(softc);
		return -1;
	}

	for(i = 0; i < 512; i += 2) {
		unsigned short n = bus_space_read16(&softc->wdc->space, WD_DR);

		buffer[i + 0] = (n >> 8) & 0xff;
		buffer[i + 1] = (n >> 0) & 0xff;
	}

	nsectors = (((unsigned int)buffer[122] << 8 | buffer[123]) << 16) | ((unsigned int)buffer[120] << 8 | buffer[121]);
	if(nsectors == 0 || nsectors > 0x0fffffff) {
		spinlock_unlock(&softc->wdc->lock);
		free(softc);
		return -1;
	}
	spinlock_unlock(&softc->wdc->lock);

	strcpy(self->desc, "<");
	memcpy(self->desc + 1, buffer + 54, 40);
	self->desc[41] = 0;
	for(i = 40; i >= 1; i--) {
		if(self->desc[i] != ' ') break;
		self->desc[i] = 0;
	}

	strcat(self->desc, ">");

	if(diskdev_init(self, NULL, &wd_disk_ops, nsectors) == DEVICE_ERROR) {
		free(softc);
		return -1;
	}

	return 0;
}

static void wd_detach(struct device* self) {
	struct wd_softc* softc = self->softc;

	diskdev_detach(&softc->disk);

	free(softc);
}

struct device_driver wd_driver = {
    "wd",
    wd_match,
    wd_attach,
    NULL,
    wd_detach,
    NULL};
