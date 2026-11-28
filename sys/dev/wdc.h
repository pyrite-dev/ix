#ifndef __WDC_H__
#define __WDC_H__

#include <sys/bus.h>
#include <sys/spinlock.h>

struct wdc_softc {
	bus_space_t space;

	spinlock_t lock;
};

#endif
