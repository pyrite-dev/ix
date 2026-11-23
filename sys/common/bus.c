#include <sys/bus.h>
#include <stdlib.h>

/* TODO: Add I/O */

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

unsigned char bus_space_read8(bus_space_t* space, paddr_t offset) {
	int			stride = bus_space_stride(space, 1);
	volatile unsigned char* ptr    = (volatile unsigned char*)(space->addr + offset * stride);

	return ptr[bus_space_index(space, 1)];
}

void bus_space_write8(bus_space_t* space, paddr_t offset, unsigned char value) {
	int			stride = bus_space_stride(space, 1);
	volatile unsigned char* ptr    = (volatile unsigned char*)(space->addr + offset * stride);

	ptr[bus_space_index(space, 1)] = value;
}

unsigned short bus_space_read16(bus_space_t* space, paddr_t offset) {
	int			 stride = bus_space_stride(space, 2);
	volatile unsigned short* ptr	= (volatile unsigned short*)(space->addr + offset * stride);

	return ptr[bus_space_index(space, 2)];
}

void bus_space_write16(bus_space_t* space, paddr_t offset, unsigned short value) {
	int			 stride = bus_space_stride(space, 2);
	volatile unsigned short* ptr	= (volatile unsigned short*)(space->addr + offset * stride);

	ptr[bus_space_index(space, 2)] = value;
}

unsigned int bus_space_read32(bus_space_t* space, paddr_t offset) {
	int		       stride = bus_space_stride(space, 4);
	volatile unsigned int* ptr    = (volatile unsigned int*)(space->addr + offset * stride);

	return ptr[bus_space_index(space, 4)];
}

void bus_space_write32(bus_space_t* space, paddr_t offset, unsigned int value) {
	int		       stride = bus_space_stride(space, 4);
	volatile unsigned int* ptr    = (volatile unsigned int*)(space->addr + offset * stride);

	ptr[bus_space_index(space, 4)] = value;
}
