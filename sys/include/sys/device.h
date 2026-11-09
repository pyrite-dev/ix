#ifndef __SYS_DEVICE_H__
#define __SYS_DEVICE_H__

#include <sys/cdefs.h>

/* DO NOT REMOVE THE COMMENTS BELOW! */
/* locator: addr */
/* locator: align */
/* locator: io */
/* locator: irq */

enum DEVICE_LOCATOR {
	LOCATOR_ADDR = 0,
	LOCATOR_ALIGN,
	LOCATOR_IO,
	LOCATOR_IRQ,
	LOCATOR_COUNT
};

struct device_driver;

struct device {
	const char* name;
	unsigned int unit;

	struct device* parent;
	struct device_driver* driver;

	void* softc;
};

struct device_attach_args {
	long locators[LOCATOR_COUNT];
};

struct device_driver {
	const char* name;

	int(*match)(struct device* parent, struct device_attach_args* args);
	int(*attach)(struct device* parent, struct device* self, struct device_attach_args* args);
};

struct device_config {
	const char* name;
	unsigned int unit;
	const char* parent;

	struct device_driver* driver;

	long locators[LOCATOR_COUNT];
};

__BEGIN_EXTERN_C__

extern struct device_config devices[];

__END_EXTERN_C__

#endif
