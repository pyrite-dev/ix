#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/bus.h>
#include <sys/sched.h>
#include <sys/time.h>

#include "pit.h"

#include <stdlib.h>
#include <string.h>

struct pit_softc {
	bus_space_t space;

	unsigned int vector;
};

static int pit_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int pit_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct pit_softc* softc;
	int		  clock = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_CLOCK], 1931816);
	int		  speed = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_SPEED], 100000);
	int		  div;

	if(speed == 0) {
		return -1;
	}

	div = clock / speed;

	if(div == 0 || div > 65535) {
		return -1;
	}

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	bus_space_init(&softc->space, args->locators);

	bus_space_write8(&softc->space, PIT_MC, PIT_MC_CH0 | PIT_MC_LOHI | PIT_MC_MODE2 | PIT_MC_BIN);
	bus_space_write8(&softc->space, PIT_CH0, (div >> 0) & 0xff);
	bus_space_write8(&softc->space, PIT_CH0, (div >> 8) & 0xff);

	hz = clock / div;

	softc->vector = _intr_vector(args->locators[DEVICE_LOCATOR_IRQ]);

	return 0;
}

static void pit_enable_ints(struct device* self) {
	struct pit_softc* softc = self->softc;

	_intr_set(softc->vector, _sched_intr);
}

struct device_driver pit_driver = {
    "pit",
    pit_match,
    pit_attach,
    pit_enable_ints,
    NULL,
    NULL};
