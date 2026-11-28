#include <sys/device.h>
#include <sys/bus.h>
#include <sys/spinlock.h>

#include "wdc.h"

#include <stdlib.h>
#include <string.h>

static int wdc_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int wdc_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct wdc_softc* softc;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	bus_space_init(&softc->space, args->locators);
	spinlock_init(&softc->lock);

	return 0;
}

static void wdc_detach(struct device* self) {
	struct wdc_softc* softc = self->softc;

	free(softc);
}

struct device_driver wdc_driver = {
    "wdc",
    wdc_match,
    wdc_attach,
    NULL,
    wdc_detach,
    NULL};
