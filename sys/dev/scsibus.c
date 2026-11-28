#include <sys/device.h>

#include "scsi.h"

#include <stdlib.h>
#include <string.h>

struct scsibus_softc {
	struct device*	     hba;
	struct scsi_adapter* adapter;
};

int scsibus_exec(struct device* bus, unsigned int target, unsigned int lun, const unsigned char* cdb, unsigned int cdb_len, unsigned char* data, unsigned int data_len, int data_read) {
	struct scsibus_softc* softc = bus->softc;

	return softc->adapter->exec(softc->hba, target, lun, cdb, cdb_len, data, data_len, data_read);
}

static int scsibus_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int scsibus_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct scsibus_softc* softc;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	softc->hba     = parent;
	softc->adapter = (struct scsi_adapter*)parent->softc;

	return 0;
}

static int scsibus_occupied(struct device* self, unsigned int target, unsigned int lun) {
	struct device* child = self->first_child;

	while(child != NULL) {
		if(child->locators[DEVICE_LOCATOR_TARGET] == target && child->locators[DEVICE_LOCATOR_LUN] == lun) return 1;

		child = child->next;
	}

	return 0;
}

static void scsibus_scan(struct device* self) {
	unsigned char inquiry[36];
	unsigned int  target;
	unsigned int  lun;

	for(target = 0; target < 8; target++) {
		if(target == 7) continue;

		for(lun = 0; lun < 8; lun++) {
			unsigned char cdb[6];
			paddr_t	      locators[DEVICE_LOCATOR_COUNT];
			int	      i;

			cdb[0] = SCSI_OP_INQUIRY;
			cdb[1] = (lun & 7) << 5;
			cdb[2] = 0;
			cdb[3] = 0;
			cdb[4] = sizeof(inquiry);
			cdb[5] = 0;

			if(scsibus_exec(self, target, lun, cdb, sizeof(cdb), inquiry, sizeof(inquiry), 1) != SCSI_STATUS_GOOD) break;

			if((inquiry[0] & 0x1f) != 0) continue;

			if(scsibus_occupied(self, target, lun)) continue;

			for(i = 0; i < DEVICE_LOCATOR_COUNT; i++) locators[i] = DEVICE_LOCATOR_UNSET;
			locators[DEVICE_LOCATOR_TARGET] = target;
			locators[DEVICE_LOCATOR_LUN]	= lun;

			device_attach_any(self, "sd", locators);
		}
	}
}

static void scsibus_detach(struct device* self) {
	struct scsibus_softc* softc = self->softc;

	free(softc);
}

struct device_driver scsibus_driver = {
    "scsibus",
    scsibus_match,
    scsibus_attach,
    NULL,
    scsibus_detach,
    scsibus_scan};
