#include <sys/cdev.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/major.h>
#include <sys/ttydev.h>

#include <dyntypes.h>

struct cdevsw_map {
	unsigned int   key;
	struct cdevsw* value;
};

struct device_map {
	dev_t	       key;
	struct device* value;
};

static struct cdevsw_map* cdevsw_map;
static struct device_map* device_map;

void cdev_init(void) {
	cdevsw_map = NULL;
	hmdefault(cdevsw_map, NULL);

	device_map = NULL;
	hmdefault(device_map, NULL);

	cdev_register(CDEV_MAJOR_TTY, &tty_cdevsw);
}

int cdev_register(unsigned int major, struct cdevsw* cdevsw) {
	if(hmgeti(cdevsw_map, major) != -1) return -1;

	hmput(cdevsw_map, major, cdevsw);

	return 0;
}

void cdev_unregister(unsigned int major) {
	hmdel(cdevsw_map, major);
}

struct cdevsw* cdev_lookup(unsigned int major) {
	return hmget(cdevsw_map, major);
}

dev_t cdev_register_device(dev_t dev, struct device* device) {
	if(DEVICE_MINOR(dev) == DEVICE_MINOR_ANY) {
		unsigned int minor = 0;

		do {
			dev = DEVICE_MAKE(DEVICE_MAJOR(dev), minor);
			minor++;
		} while(minor != DEVICE_MINOR_ANY && hmgeti(device_map, dev) != -1);

		if(hmgeti(device_map, dev) != -1) return DEVICE_ERROR;
	}

	hmput(device_map, dev, device);

	return dev;
}

void cdev_unregister_device(dev_t dev) {
	hmdel(device_map, dev);
}

struct device* cdev_lookup_device(dev_t dev) {
	return hmget(device_map, dev);
}
