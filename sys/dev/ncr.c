#include <sys/device.h>
#include <sys/bus.h>
#include <sys/mutex.h>

#include "ncr.h"
#include "scsi.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NCR_OWN_ID 7

/*
 * The i960 NCR path latches data in the input register; the Mac PIO path reads
 * the current SCSI data register instead.
 */
#ifdef __i960__
#define NCR_RX_REG NCR_IDATA
#else
#define NCR_RX_REG NCR_DATA
#endif

/* A target answers selection in microseconds; missing IDs must not stall boot. */
#define NCR_SELECT_TIMEOUT 2000

/* Data/status phases can take longer, especially on CD-ROM drives. */
#define NCR_XFER_TIMEOUT 20000

struct ncr_softc {
	struct scsi_adapter adapter;

	bus_space_t rspace;
	bus_space_t wspace;

	mutex_t lock;
};

static void ncr_delay(void) {
	volatile int i;

	for(i = 0; i < 8; i++);
}

static uint8_t ncr_read(struct ncr_softc* softc, int reg) {
	return bus_space_read8(&softc->rspace, reg);
}

static void ncr_write(struct ncr_softc* softc, int reg, uint8_t value) {
	bus_space_write8(&softc->wspace, reg, value);
	ncr_delay();
}

static int ncr_wait_reg(struct ncr_softc* softc, int reg, uint8_t mask, uint8_t value, int timeout) {
	volatile int i;

	for(i = 0; i < timeout; i++) {
		if((ncr_read(softc, reg) & mask) == value) return 0;
	}

	return -1;
}

static int ncr_wait_csr(struct ncr_softc* softc, uint8_t mask, uint8_t value, int timeout) {
	return ncr_wait_reg(softc, NCR_CSR, mask, value, timeout);
}

static int ncr_wait_req(struct ncr_softc* softc) {
	return ncr_wait_csr(softc, NCR_CSR_REQ, NCR_CSR_REQ, NCR_XFER_TIMEOUT);
}

static void ncr_ack(struct ncr_softc* softc) {
	ncr_write(softc, NCR_ICR, NCR_ICR_ACK);
	ncr_write(softc, NCR_ICR, 0);
}

static void ncr_bus_delay(void) {
	int i;

	for(i = 0; i < 8; i++) ncr_delay();
}

static void ncr_reset(struct ncr_softc* softc) {
	ncr_write(softc, NCR_ICR, NCR_ICR_RST);
	ncr_bus_delay();
	ncr_write(softc, NCR_ICR, 0);
	ncr_write(softc, NCR_MODE, 0);
	ncr_write(softc, NCR_CSR, 0);
	ncr_bus_delay();
}

static void ncr_select_release(struct ncr_softc* softc) {
	ncr_write(softc, NCR_ICR, 0);
	ncr_write(softc, NCR_MODE, 0);
	ncr_write(softc, NCR_CSR, 0);
}

static int ncr_select(struct ncr_softc* softc, int target) {
	ncr_write(softc, NCR_TCR, 0);

	/* arbitrate as ID 7 */
	ncr_write(softc, NCR_DATA, 1 << NCR_OWN_ID);
	ncr_write(softc, NCR_MODE, NCR_MODE_ARBITRATE);

	if(ncr_wait_reg(softc, NCR_ICR, NCR_ICR_AIP, NCR_ICR_AIP, NCR_SELECT_TIMEOUT) != 0) {
		ncr_select_release(softc);
		return -1;
	}

	ncr_delay();

	if(ncr_read(softc, NCR_ICR) & NCR_ICR_LA) {
		ncr_select_release(softc);
		return -1;
	}

	ncr_write(softc, NCR_ICR, NCR_ICR_SEL);

	if(ncr_read(softc, NCR_ICR) & NCR_ICR_LA) {
		ncr_select_release(softc);
		return -1;
	}

	ncr_delay();

	/* assert the initiator and target IDs while holding SEL */
	ncr_write(softc, NCR_DATA, (1 << NCR_OWN_ID) | (1 << target));
	ncr_write(softc, NCR_ICR, NCR_ICR_BSY | NCR_ICR_DATA | NCR_ICR_SEL);
	ncr_write(softc, NCR_MODE, 0);
	ncr_write(softc, NCR_CSR, 0);
	ncr_delay();
	ncr_write(softc, NCR_ICR, NCR_ICR_DATA | NCR_ICR_SEL);
	ncr_delay();

	if(ncr_wait_csr(softc, NCR_CSR_BSY, NCR_CSR_BSY, NCR_SELECT_TIMEOUT) != 0) {
		ncr_select_release(softc);
		return -1;
	}

	ncr_delay();
	ncr_write(softc, NCR_ICR, 0);

	return 0;
}

static void ncr_send(struct ncr_softc* softc, uint8_t value) {
	ncr_write(softc, NCR_DATA, value);
	ncr_ack(softc);
}

static uint8_t ncr_recv(struct ncr_softc* softc) {
	uint8_t value = ncr_read(softc, NCR_DATA);

	ncr_ack(softc);

	return value;
}

static uint8_t ncr_recv_data(struct ncr_softc* softc) {
	uint8_t value = ncr_read(softc, NCR_RX_REG);

	ncr_ack(softc);

	return value;
}

static int ncr_run(struct ncr_softc* softc, int target, const uint8_t* cmd, int cmdlen, void* data_, int datalen, int read) {
	uint8_t* data = data_;
	uint8_t	 status = 0xff;
	uint8_t	 message = 0xff;
	int	 result;
	int	 i;

	mutex_lock(&softc->lock);

	if(ncr_select(softc, target) != 0) {
		result = -1;
		goto out;
	}

	for(i = 0; i < cmdlen; i++) {
		if(ncr_wait_req(softc) != 0) goto fail;
		ncr_send(softc, cmd[i]);
	}

	for(i = 0; i < datalen; i++) {
		uint8_t phase;

		if(ncr_wait_req(softc) != 0) goto fail;

		phase = NCR_PHASE(ncr_read(softc, NCR_CSR));

		if(read) {
			if(phase != NCR_PHASE_DATA_IN) goto status;
			data[i] = ncr_recv_data(softc);
		} else {
			if(phase != NCR_PHASE_DATA_OUT) goto status;
			ncr_send(softc, data[i]);
		}
	}

status:
	if(ncr_wait_req(softc) != 0) goto fail;
	if(NCR_PHASE(ncr_read(softc, NCR_CSR)) != NCR_PHASE_STATUS) goto fail;
	status = ncr_recv(softc);

	if(ncr_wait_req(softc) == 0) {
		if(NCR_PHASE(ncr_read(softc, NCR_CSR)) == NCR_PHASE_MESSAGE_IN) message = ncr_recv(softc);
	}

	ncr_wait_csr(softc, NCR_CSR_BSY, 0, NCR_XFER_TIMEOUT);

	result = status == 0 && message == 0 ? 0 : -1;
	goto out;

fail:
	ncr_reset(softc);
	result = -1;

out:
	mutex_unlock(&softc->lock);

	return result;
}

static int ncr_exec(struct device* hba, unsigned int target, unsigned int lun, const unsigned char* cdb, unsigned int cdb_len, unsigned char* data, unsigned int data_len, int data_read) {
	struct ncr_softc* softc = hba->softc;

	(void)lun;

	return ncr_run(softc, target, cdb, cdb_len, data, data_len, data_read);
}

static int ncr_probe(struct ncr_softc* softc) {
	static const unsigned char tcr_patterns[]  = {0x05, 0x0a};
	static const unsigned char mode_patterns[] = {0x24, 0x18};
	unsigned int		   i;
	int			   ok = 1;

	for(i = 0; i < sizeof(tcr_patterns) && ok; i++) {
		bus_space_write8(&softc->wspace, NCR_TCR, tcr_patterns[i]);
		if(bus_space_read8(&softc->rspace, NCR_TCR) != tcr_patterns[i]) ok = 0;
	}

	for(i = 0; i < sizeof(mode_patterns) && ok; i++) {
		bus_space_write8(&softc->wspace, NCR_MODE, mode_patterns[i]);
		if(bus_space_read8(&softc->rspace, NCR_MODE) != mode_patterns[i]) ok = 0;
	}

	bus_space_write8(&softc->wspace, NCR_TCR, 0);
	bus_space_write8(&softc->wspace, NCR_MODE, 0);

	return ok;
}

static int ncr_match(struct device* parent, struct device_attach_args* args) {
#ifdef __m68k__
	volatile uint32_t* rombase = (volatile uint32_t*)0x02ae;
	volatile uint8_t*  rom	    = (volatile uint8_t*)(uintptr_t)*rombase;

	/* SCSI is only present on the Mac Plus and later (ROM version > 0x69) */
	return rom[9] > 0x69;
#else
	return 1;
#endif
}

static int ncr_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct ncr_softc* softc;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	mutex_init(&softc->lock);

	bus_space_init(&softc->rspace, args->locators);
	bus_space_init(&softc->wspace, args->locators);

	/* memory-mapped device: the port locator names the write window */
	softc->rspace.port = DEVICE_LOCATOR_UNSET;
	softc->wspace.port = DEVICE_LOCATOR_UNSET;

	softc->wspace.addr = args->locators[DEVICE_LOCATOR_PORT] == DEVICE_LOCATOR_UNSET
				 ? args->locators[DEVICE_LOCATOR_ADDR]
				 : args->locators[DEVICE_LOCATOR_PORT];

	if(!ncr_probe(softc)) {
		free(softc);
		self->softc = NULL;
		return -1;
	}

	softc->adapter.exec = ncr_exec;

	ncr_write(softc, NCR_ICR, 0);
	ncr_write(softc, NCR_MODE, 0);
	ncr_write(softc, NCR_CSR, 0);

	if(ncr_read(softc, NCR_CSR) & (NCR_CSR_BSY | NCR_CSR_SEL | NCR_CSR_RST)) {
		ncr_reset(softc);
		ncr_wait_csr(softc, NCR_CSR_BSY, 0, NCR_XFER_TIMEOUT);
	}

	return 0;
}

static void ncr_detach(struct device* self) {
	struct ncr_softc* softc = self->softc;

	if(softc == NULL) return;

	free(softc);
	self->softc = NULL;
}

struct device_driver ncr_driver = {
    "ncr",
    ncr_match,
    ncr_attach,
    NULL,
    ncr_detach,
    NULL};
