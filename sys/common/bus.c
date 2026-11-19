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

static paddr_t bus_space_addr(bus_space_t* space, paddr_t offset, int def) {
	int	stride = bus_space_stride(space, def);
	paddr_t addr   = space->addr;

	if(space->stride != DEVICE_LOCATOR_UNSET) addr -= addr % stride;

	return addr + offset * stride;
}

static int bus_space_index(bus_space_t* space, int def) {
	int stride = bus_space_stride(space, def);

	if(space->stride == DEVICE_LOCATOR_UNSET) return 0;

	return (space->addr % stride) / def;
}

#define BUS(type, sz, n) \
	type bus_space_read##n(bus_space_t* space, paddr_t offset) { \
		volatile type* ptr; \
		if(space->port != DEVICE_LOCATOR_UNSET) { \
			return in##sz(space->port + offset); \
		} \
\
		ptr = (volatile type*)bus_space_addr(space, offset, sizeof(type)); \
\
		return ptr[bus_space_index(space, sizeof(type))]; \
	} \
\
	void bus_space_write##n(bus_space_t* space, paddr_t offset, type value) { \
		volatile type* ptr; \
		if(space->port != DEVICE_LOCATOR_UNSET) { \
			out##sz(space->port + offset, value); \
\
			return; \
		} \
\
		ptr = (volatile type*)bus_space_addr(space, offset, sizeof(type)); \
\
		ptr[bus_space_index(space, sizeof(type))] = value; \
	}

BUS(unsigned char, b, 8);
BUS(unsigned short, w, 16);
BUS(unsigned int, l, 32);
