#ifndef __SYS_DEVICE_H__
#define __SYS_DEVICE_H__

#include <sys/cdefs.h>
#include <sys/types.h>

/* DO NOT REMOVE THE COMMENTS BELOW! */
/* locator: addr */
/* locator: port */
/* locator: irq */
/* locator: stride */

enum DEVICE_LOCATOR {
	DEVICE_LOCATOR_ADDR = 0,
	DEVICE_LOCATOR_PORT,
	DEVICE_LOCATOR_IRQ,
	DEVICE_LOCATOR_STRIDE,
	DEVICE_LOCATOR_COUNT
};

#define DEVICE_LOCATORS { \
    "addr", "%lx", \
    "port", "%lx", \
    "irq", "%ld", \
    "stride", "%ld"}

#define DEVICE_LOCATOR_UNSET ((paddr_t) - 1)

#define DEVICE_UNIT_ANY (-1)

struct device_driver;

struct device {
	char*	     name;
	unsigned int unit;

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

extern struct device_config devices[];

void device_init(void);

__END_EXTERN_C__

#endif
