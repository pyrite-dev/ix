#include <sys/device.h>
#include <sys/time.h>

#include "scsi.h"
#include "wd.h"
#include "wdc.h"

#include <stdlib.h>
#include <string.h>

#define ATAPI_C_PACKET 0xa0   /* PACKET */
#define ATAPI_C_IDENTIFY 0xa1 /* IDENTIFY PACKET DEVICE */
#define ATAPI_DH_ALWAYS ((1 << 7) | (1 << 5))
#define ATAPI_SIG_MID 0x14
#define ATAPI_SIG_HIGH 0xeb
#define ATAPI_TYPE_CDROM 5

struct atapi_softc {
	struct scsi_transport transport;

	struct wdc_softc* wdc;

	unsigned int drive;
	int	     type;
};

static int wait_bsy(bus_space_t* space) {
	unsigned long limit = ticktime(100);
	unsigned long start = ticks;
	unsigned char status;

	while(1) {
		status = bus_space_read8(space, WD_SR);

		if(!(status & WD_S_BSY)) return 0;
		if(ticks - start >= limit) return -1;
	}
}

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

static int atapi_exec(struct device* bus, unsigned int target, unsigned int lun, const unsigned char* cdb, unsigned int cdb_len, unsigned char* data, unsigned int data_len, int data_read) {
	struct atapi_softc* softc = bus->softc;
	struct wdc_softc*   wdc	  = softc->wdc;
	bus_space_t*	    space = &wdc->space;
	unsigned char	    packet[12];
	unsigned int	    done = 0;
	unsigned int	    i;
	int		    result;

	memset(packet, 0, sizeof(packet));
	if(cdb_len > sizeof(packet)) cdb_len = sizeof(packet);
	memcpy(packet, cdb, cdb_len);

	spinlock_lock(&wdc->lock);

	bus_space_write8(space, WD_DHR, (softc->drive ? WD_DH_SLAVE : WD_DH_MASTER) | ATAPI_DH_ALWAYS);

	if(wait_bsy(space) != 0) {
		result = -1;
		goto out;
	}

	bus_space_write8(space, WD_FR, 0);
	bus_space_write8(space, WD_LBAMR, 0xff);
	bus_space_write8(space, WD_LBAHR, 0xff);
	bus_space_write8(space, WD_CR, ATAPI_C_PACKET);

	if(wait_drq(space) != 0) {
		result = SCSI_STATUS_CHECK_CONDITION;
		goto out;
	}

	for(i = 0; i < sizeof(packet); i += 2) bus_space_write16(space, WD_DR, packet[i] | (packet[i + 1] << 8));

	while(done < data_len) {
		unsigned int count;

		if(wait_drq(space) != 0) {
			result = SCSI_STATUS_CHECK_CONDITION;
			goto out;
		}

		count = bus_space_read8(space, WD_LBAMR) | (bus_space_read8(space, WD_LBAHR) << 8);
		if(count == 0 || count > data_len - done) count = data_len - done;

		if(data_read) {
			for(i = 0; i + 1 < count; i += 2) {
				unsigned short v = bus_space_read16(space, WD_DR);

				data[done + i]	   = v & 0xff;
				data[done + i + 1] = (v >> 8) & 0xff;
			}
			if(i < count) data[done + i] = bus_space_read8(space, WD_DR);
		} else {
			for(i = 0; i + 1 < count; i += 2) bus_space_write16(space, WD_DR, data[done + i] | (data[done + i + 1] << 8));
			if(i < count) bus_space_write8(space, WD_DR, data[done + i]);
		}

		done += count;
	}

	if(wait_bsy(space) != 0) {
		result = -1;
		goto out;
	}

	result = (bus_space_read8(space, WD_SR) & WD_S_ERR) ? SCSI_STATUS_CHECK_CONDITION : SCSI_STATUS_GOOD;

out:
	spinlock_unlock(&wdc->lock);

	return result;
}

static int atapi_probe(struct wdc_softc* wdc, unsigned int drive) {
	bus_space_t*  space = &wdc->space;
	unsigned char ident[512];
	unsigned int  i;
	int	      type = -1;

	spinlock_lock(&wdc->lock);

	bus_space_write8(space, WD_DHR, (drive ? WD_DH_SLAVE : WD_DH_MASTER) | ATAPI_DH_ALWAYS);

	if(wait_bsy(space) != 0) goto out;

	bus_space_write8(space, WD_LBAMR, 0);
	bus_space_write8(space, WD_LBAHR, 0);
	bus_space_write8(space, WD_CR, ATAPI_C_IDENTIFY);

	if(wait_bsy(space) != 0) goto out;

	if(bus_space_read8(space, WD_LBAMR) != ATAPI_SIG_MID) goto out;
	if(bus_space_read8(space, WD_LBAHR) != ATAPI_SIG_HIGH) goto out;

	if(wait_drq(space) != 0) goto out;

	for(i = 0; i < 256; i++) {
		unsigned short v = bus_space_read16(space, WD_DR);

		ident[2 * i]	 = v & 0xff;
		ident[2 * i + 1] = (v >> 8) & 0xff;
	}

	type = ((ident[0] | (ident[1] << 8)) >> 8) & 0x1f;

out:
	spinlock_unlock(&wdc->lock);

	return type;
}

static int atapi_occupied(struct device* self, unsigned int drive) {
	struct device* child = self->first_child;

	while(child != NULL) {
		if(child->locators[DEVICE_LOCATOR_TARGET] == drive && child->locators[DEVICE_LOCATOR_LUN] == 0) return 1;

		child = child->next;
	}

	return 0;
}

static void atapi_scan(struct device* self) {
	struct atapi_softc* softc = self->softc;
	paddr_t		    locators[DEVICE_LOCATOR_COUNT];
	int		    i;

	if(softc->type != ATAPI_TYPE_CDROM) return;

	if(atapi_occupied(self, softc->drive)) return;

	for(i = 0; i < DEVICE_LOCATOR_COUNT; i++) locators[i] = DEVICE_LOCATOR_UNSET;
	locators[DEVICE_LOCATOR_TARGET] = softc->drive;
	locators[DEVICE_LOCATOR_LUN]	= 0;

	device_attach_any(self, "cd", locators);
}

static int atapi_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int atapi_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct atapi_softc* softc;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	softc->transport.exec = atapi_exec;
	softc->wdc	      = (struct wdc_softc*)parent->softc;
	softc->drive	      = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_DRIVE], 0);

	softc->type = atapi_probe(softc->wdc, softc->drive);
	if(softc->type < 0) {
		free(softc);
		return -1;
	}

	return 0;
}

static void atapi_detach(struct device* self) {
	struct atapi_softc* softc = self->softc;

	free(softc);
}

struct device_driver atapi_driver = {
    "atapi",
    atapi_match,
    atapi_attach,
    NULL,
    atapi_detach,
    atapi_scan};
