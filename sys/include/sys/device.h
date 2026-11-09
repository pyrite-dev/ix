#ifndef __SYS_DEVICE_H__
#define __SYS_DEVICE_H__

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

struct device_config {
	const char* name;
	unsigned int unit;
	const char* parent;
	long locators[LOCATOR_COUNT];
};

#endif
