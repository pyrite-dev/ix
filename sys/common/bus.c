#include <sys/bus.h>
#include <sys/io.h>

#include <stdlib.h>

void bus_space_init(bus_space_t* space, paddr_t locators[DEVICE_LOCATOR_COUNT]) {
	space->port   = locators[DEVICE_LOCATOR_PORT];
	space->addr   = locators[DEVICE_LOCATOR_ADDR];
	space->stride = locators[DEVICE_LOCATOR_STRIDE];
}

static int bus_space_stride(bus_space_t* space, int def) {
	return space->stride == DEVICE_LOCATOR_UNSET ? def : space->stride;
}

/* TODO: Make this function calculate actual index */
static int bus_space_index(bus_space_t* space, int def) {
	int stride = bus_space_stride(space, def);

	if((stride % def) == 0) {
#ifdef __LITTLE_ENDIAN__
		return 0;
#else
		return (stride / def) - 1;
#endif
	}

	/* FIXME */
	return 0;
}

#define BUS(type, sz, n) \
	type bus_space_read##n(bus_space_t* space, paddr_t offset) { \
		int	       stride = bus_space_stride(space, sizeof(type)); \
		volatile type* ptr; \
		if(space->port != DEVICE_LOCATOR_UNSET) { \
			return in##sz(space->port + offset); \
		} \
\
		ptr = (volatile type*)(space->addr + offset * stride); \
\
		return ptr[bus_space_index(space, sizeof(type))]; \
	} \
\
	void bus_space_write##n(bus_space_t* space, paddr_t offset, type value) { \
		int	       stride = bus_space_stride(space, sizeof(type)); \
		volatile type* ptr; \
		if(space->port != DEVICE_LOCATOR_UNSET) { \
			out##sz(space->port + offset, value); \
\
			return; \
		} \
\
		ptr = (volatile type*)(space->addr + offset * stride); \
\
		ptr[bus_space_index(space, sizeof(type))] = value; \
	}

BUS(unsigned char, b, 8);
BUS(unsigned short, w, 16);
BUS(unsigned int, l, 32);
