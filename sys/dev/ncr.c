#include <sys/device.h>
#include <sys/bus.h>
#include <sys/spinlock.h>

#include "ncr.h"
#include "scsi.h"

#include <stdlib.h>
#include <string.h>

struct ncr_softc {
	struct scsi_adapter adapter;

	bus_space_t space;
	spinlock_t  lock;
};

#define NCR_SELECT_TIMEOUT 100000
#define NCR_PHASE_GUARD 10000

static void ncr_reset(struct ncr_softc* softc) {
	bus_space_write8(&softc->space, NCR_ICR, NCR_ICR_RST);
	bus_space_write8(&softc->space, NCR_ICR, 0);
}

static void ncr_ack(struct ncr_softc* softc) {
	bus_space_write8(&softc->space, NCR_ICR, NCR_ICR_ACK);
	bus_space_write8(&softc->space, NCR_ICR, 0);
}

static int ncr_probe(struct ncr_softc* softc) {
	static const unsigned char tcr_patterns[]  = {0x05, 0x0a};
	static const unsigned char mode_patterns[] = {0x24, 0x18};
	unsigned int		     i;
	int			     ok = 1;

	for(i = 0; i < sizeof(tcr_patterns) && ok; i++) {
		bus_space_write8(&softc->space, NCR_TCR, tcr_patterns[i]);
		if(bus_space_read8(&softc->space, NCR_TCR) != tcr_patterns[i]) ok = 0;
	}

	for(i = 0; i < sizeof(mode_patterns) && ok; i++) {
		bus_space_write8(&softc->space, NCR_MODE, mode_patterns[i]);
		if(bus_space_read8(&softc->space, NCR_MODE) != mode_patterns[i]) ok = 0;
	}

	bus_space_write8(&softc->space, NCR_TCR, 0);
	bus_space_write8(&softc->space, NCR_MODE, 0);

	return ok;
}

static int ncr_exec(struct device* hba, unsigned int target, unsigned int lun, const unsigned char* cdb, unsigned int cdb_len, unsigned char* data, unsigned int data_len, int data_read) {
	struct ncr_softc* softc = hba->softc;
	unsigned int	  i;
	int		  guard;
	unsigned char	  csr;
	unsigned char	  status;
	unsigned char	  msg;

	spinlock_lock(&softc->lock);

	bus_space_write8(&softc->space, NCR_DATA, (1 << target) | (1 << 7));
	bus_space_write8(&softc->space, NCR_ICR, NCR_ICR_SEL | NCR_ICR_DATA);

	for(i = 0; i < NCR_SELECT_TIMEOUT; i++) {
		if(bus_space_read8(&softc->space, NCR_CSR) & NCR_CSR_BSY) break;
	}
	if(i == NCR_SELECT_TIMEOUT) {
		bus_space_write8(&softc->space, NCR_ICR, 0);
		spinlock_unlock(&softc->lock);
		return -1;
	}

	bus_space_write8(&softc->space, NCR_ICR, 0);

	for(i = 0; i < cdb_len; i++) {
		bus_space_write8(&softc->space, NCR_DATA, cdb[i]);
		ncr_ack(softc);
	}

	for(guard = 0; guard < NCR_PHASE_GUARD; guard++) {
		csr = bus_space_read8(&softc->space, NCR_CSR);

		if(NCR_PHASE(csr) == NCR_PHASE_STATUS) break;

		if(NCR_PHASE(csr) == NCR_PHASE_DATA_IN) {
			if(!data_read) goto error;

			for(i = 0; i < data_len; i++) {
				data[i] = bus_space_read8(&softc->space, NCR_IDATA);
				ncr_ack(softc);
			}
		} else if(NCR_PHASE(csr) == NCR_PHASE_DATA_OUT) {
			if(data_read) goto error;

			for(i = 0; i < data_len; i++) {
				bus_space_write8(&softc->space, NCR_DATA, data[i]);
				ncr_ack(softc);
			}
		} else {
			goto error;
		}
	}
	if(guard == NCR_PHASE_GUARD) goto error;

	status = bus_space_read8(&softc->space, NCR_DATA);
	ncr_ack(softc);

	csr = bus_space_read8(&softc->space, NCR_CSR);
	if(NCR_PHASE(csr) != NCR_PHASE_MESSAGE_IN) goto error;

	msg = bus_space_read8(&softc->space, NCR_DATA);
	if(msg != 0) goto error;

	ncr_ack(softc);

	if(bus_space_read8(&softc->space, NCR_CSR) & NCR_CSR_BSY) goto error;

	spinlock_unlock(&softc->lock);

	return status;

error:
	ncr_reset(softc);
	spinlock_unlock(&softc->lock);
	return -1;
}

static int ncr_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int ncr_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct ncr_softc* softc;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	bus_space_init(&softc->space, args->locators);

	if(!ncr_probe(softc)) {
		free(softc);
		return -1;
	}

	spinlock_init(&softc->lock);

	softc->adapter.exec = ncr_exec;

	ncr_reset(softc);

	return 0;
}

static void ncr_detach(struct device* self) {
	struct ncr_softc* softc = self->softc;

	free(softc);
}

struct device_driver ncr_driver = {
    "ncr",
    ncr_match,
    ncr_attach,
    NULL,
    ncr_detach};
