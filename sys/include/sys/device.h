#ifndef __SYS_DEVICE_H__
#define __SYS_DEVICE_H__

struct device;
struct device_attach_args;
struct device_driver;

enum DEVICE_LOCATOR {
	DEVICE_LOCATOR_ADDR = 0,
	DEVICE_LOCATOR_PORT,
	DEVICE_LOCATOR_IRQ,
	DEVICE_LOCATOR_STRIDE,
	DEVICE_LOCATOR_CLOCK,
	DEVICE_LOCATOR_SPEED,
	DEVICE_LOCATOR_BAUD,
	DEVICE_LOCATOR_TARGET,
	DEVICE_LOCATOR_LUN,
	DEVICE_LOCATOR_COUNT
};

#define DEVICE_LOCATORS { \
    "addr", "0x%llx", \
    "port", "0x%llx", \
    "irq", "%lld", \
    "stride", "%lld", \
    "clock", "%lld", \
    "speed", "%lld", \
    "baud", "%lld", \
    "target", "%lld", \
    "lun", "%lld"}

#define DEVICE_LOCATOR_UNSET ((paddr_t) - 1)
#define DEVICE_LOCATOR_DEFAULT(loc, def) ((loc) == DEVICE_LOCATOR_UNSET ? (def) : (loc))

#define DEVICE_UNIT_ANY (-1)

#define DEVICE_MAJOR(dev) (((dev) >> 16) & 0xffff)
#define DEVICE_MINOR(dev) (((dev) >> 0) & 0xffff)
#define DEVICE_MAKE(major, minor) ((((dev_t)(major) & 0xffff) << 16) | ((dev_t)(minor) & 0xffff))
#define DEVICE_MINOR_ANY (0xffff)
#define DEVICE_ERROR ((dev_t)(-1))

#include <sys/cdefs.h>
#include <sys/types.h>

struct device {
	char*	     name;
	unsigned int unit;

	char desc[512];

	struct device* parent;
	struct device* prev;
	struct device* next;
	struct device* first_child;

	struct device_driver* driver;

	void* softc;
};

struct device_attach_args {
	paddr_t locators[DEVICE_LOCATOR_COUNT];
};

struct device_driver {
	const char* name;

	int (*match)(struct device* parent, struct device_attach_args* args);
	int (*attach)(struct device* parent, struct device* self, struct device_attach_args* args);
	void (*enable_ints)(struct device* self);
	void (*detach)(struct device* self);
};

struct device_config {
	const char* name;
	int	    unit;
	const char* parent;
	int	    parent_unit;

	struct device_driver* driver;

	paddr_t locators[DEVICE_LOCATOR_COUNT];
};

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
extern struct device_config devices[];

void device_init(void);
void device_start(void);
#endif

__END_EXTERN_C__

#endif
