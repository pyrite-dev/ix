#ifndef __SYS_BUS_H__
#define __SYS_BUS_H__

#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/types.h>

typedef struct bus_space bus_space_t;

struct bus_space {
	paddr_t port;
	paddr_t addr;
	paddr_t stride;
};

__BEGIN_EXTERN_C__

void bus_space_init(bus_space_t* space, paddr_t locators[DEVICE_LOCATOR_COUNT]);

unsigned char  bus_space_read8(bus_space_t* space, paddr_t offset);
void	       bus_space_write8(bus_space_t* space, paddr_t offset, unsigned char value);
unsigned short bus_space_read16(bus_space_t* space, paddr_t offset);
void	       bus_space_write16(bus_space_t* space, paddr_t offset, unsigned short value);
unsigned int   bus_space_read32(bus_space_t* space, paddr_t offset);
void	       bus_space_write32(bus_space_t* space, paddr_t offset, unsigned int value);

__END_EXTERN_C__

#endif
