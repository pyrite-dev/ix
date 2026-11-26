#include <sys/bdev.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/major.h>
#include <sys/diskdev.h>

#include <stdio.h>
#include <string.h>

#include <dyntypes.h>

struct bdevsw_map {
	unsigned int   key;
	struct bdevsw* value;
};

struct device_map {
	dev_t	       key;
	struct device* value;
};

static struct bdevsw_map* bdevsw_map;
static struct device_map* device_map;

void bdev_init(void) {
	bdevsw_map = NULL;
	hmdefault(bdevsw_map, NULL);

	device_map = NULL;
	hmdefault(device_map, NULL);

	bdev_register(BDEV_MAJOR_DISK, &disk_bdevsw);
}

int bdev_register(unsigned int major, struct bdevsw* bdevsw) {
	if(hmgeti(bdevsw_map, major) != -1) return -1;

	hmput(bdevsw_map, major, bdevsw);

	return 0;
}

void bdev_unregister(unsigned int major) {
	hmdel(bdevsw_map, major);
}

struct bdevsw* bdev_lookup(unsigned int major) {
	return hmget(bdevsw_map, major);
}

dev_t bdev_register_device(dev_t dev, struct device* device) {
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

void bdev_unregister_device(dev_t dev) {
	hmdel(device_map, dev);
}

struct device* bdev_lookup_device(dev_t dev) {
	return hmget(device_map, dev);
}

int bdev_find_device(const char* name, dev_t* result) {
	char	     buf[32];
	unsigned int i;

	for(i = 0; i < hmlen(device_map); i++) {
		struct device* device = device_map[i].value;

		snprintf(buf, sizeof(buf), "%s%u", device->name, device->unit);

		if(strcmp(buf, name) == 0) {
			*result = device_map[i].key;
			return 0;
		}
	}

	return -1;
}

unsigned int bdev_device_count(void) {
	return hmlen(device_map);
}

int bdev_device_at(unsigned int index, dev_t* dev, char* name, size_t name_size) {
	struct device* device;

	if(index >= hmlen(device_map)) return -1;

	device = device_map[index].value;

	snprintf(name, name_size, "%s%u", device->name, device->unit);

	*dev = device_map[index].key;

	return 0;
}
