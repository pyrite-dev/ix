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

static void scsibus_detach(struct device* self) {
	struct scsibus_softc* softc = self->softc;

	free(softc);
}

struct device_driver scsibus_driver = {
    "scsibus",
    scsibus_match,
    scsibus_attach,
    NULL,
    scsibus_detach};
