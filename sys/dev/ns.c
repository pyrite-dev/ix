#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/ins8250.h>
#include <sys/bus.h>
#include <stdlib.h>

struct ns_softc {
	bus_space_t* space;
};

static unsigned short baud_to_div(int baud) {
	return 10000000 / (16 * baud);
}

static int ns_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static void ns_intr(unsigned int vector, struct iregs* iregs, void* arg) {
}

static int ns_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	unsigned short	 div = baud_to_div(9600);
	struct ns_softc* softc;

	self->softc = softc = malloc(sizeof(*softc));
	softc->space	    = bus_space_new(args->locators);

	bus_space_write8(softc->space, IER, 0);
	bus_space_write8(softc->space, LCR, LC_DLAB);
	bus_space_write8(softc->space, DHR, (div >> 8) & 0xff);
	bus_space_write8(softc->space, DLR, (div >> 0) & 0xff);
	bus_space_write8(softc->space, LCR, LC_PB_N | LC_SB | LC_DB_8);
	bus_space_write8(softc->space, IIR, FC_ITL_14 | FC_CTFIFO | FC_CRFIFO | FC_EFIFO);
	bus_space_write8(softc->space, MCR, MC_L | MC_O2 | MC_O1 | MC_RTS);

	while(!(bus_space_read8(softc->space, LSR) & LS_THRE));
	bus_space_write8(softc->space, DR, 0xae);

	while(!(bus_space_read8(softc->space, LSR) & LS_DR));
	if(bus_space_read8(softc->space, DR) != 0xae) {
		/* serial is faulty */
		free(self->softc);
		return -1;
	}

	bus_space_write8(softc->space, MCR, MC_O2 | MC_O1 | MC_RTS | MC_DTR);

	intr_register(_intr_vector(args->locators[DEVICE_LOCATOR_IRQ]), ns_intr, self);
	return 0;
}

static void ns_detach(struct device* self) {
	free(self->softc);
}

struct device_driver ns_driver = {
    "ns",
    ns_match,
    ns_attach,
    ns_detach};
