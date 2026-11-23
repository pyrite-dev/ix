#include <sys/device.h>
#include <sys/bus.h>
#include <sys/cons.h>

#include "wd.h"

#include <stdlib.h>
#include <stddef.h>

struct wd_softc {
	bus_space_t space;
};

static void wait_drq(bus_space_t* space) {
	while(!(bus_space_read8(space, WD_SR) & WD_S_DRQ));
}

static int wd_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int wd_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct wd_softc* softc;
	unsigned char	 status;
	unsigned char	 buffer[512];
	int		 i;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	bus_space_init(&softc->space, args->locators);

	bus_space_write8(&softc->space, WD_DHR, WD_DH_MASTER | WD_DH_ALWAYS);

	bus_space_write8(&softc->space, WD_SCR, 0);
	bus_space_write8(&softc->space, WD_LBALR, 0);
	bus_space_write8(&softc->space, WD_LBAMR, 0);
	bus_space_write8(&softc->space, WD_LBAHR, 0);

	bus_space_write8(&softc->space, WD_CR, WD_C_IDENTIFY);

	status = bus_space_read8(&softc->space, WD_SR);
	if(status == 0 || status == 0xff) return -1;

	wait_drq(&softc->space);

	for(i = 0; i < 512; i += 2) {
		unsigned short n = bus_space_read16(&softc->space, WD_DR);

		buffer[i + 0] = (n >> 8) & 0xff;
		buffer[i + 1] = (n >> 0) & 0xff;
	}

	strcpy(self->desc, "<");
	memcpy(self->desc + 1, buffer + 54, 40);
	self->desc[41] = 0;
	for(i = 40; i >= 1; i--) {
		if(self->desc[i] != ' ') break;
		self->desc[i] = 0;
	}

	strcat(self->desc, ">");

	return 0;
}

static void wd_detach(struct device* self) {
	struct wd_softc* softc = self->softc;

	free(softc);
}

struct device_driver wd_driver = {
    "wd",
    wd_match,
    wd_attach,
    NULL,
    wd_detach};
