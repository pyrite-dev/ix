#include <sys/cdev.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/major.h>
#include <sys/ttydev.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dyntypes.h>

struct sw_map {
	unsigned int   key;
	struct cdevsw* value;
};

struct device_entry {
	struct device* device;
	char	       name[32];
};

struct device_map {
	dev_t		     key;
	struct device_entry* value;
};

static struct sw_map*	  sw_map;
static struct device_map* device_map;

void cdev_init(void) {
	sw_map = NULL;
	hmdefault(sw_map, NULL);

	device_map = NULL;
	hmdefault(device_map, NULL);

	cdev_register(CDEV_MAJOR_TTY, &tty_cdevsw);
}

int cdev_register(unsigned int major, struct cdevsw* sw) {
	if(hmgeti(sw_map, major) != -1) return -1;

	hmput(sw_map, major, sw);

	return 0;
}

void cdev_unregister(unsigned int major) {
	hmdel(sw_map, major);
}

struct cdevsw* cdev_lookup(unsigned int major) {
	return hmget(sw_map, major);
}

dev_t cdev_register_device(dev_t dev, struct device* device, const char* name) {
	struct device_entry* entry;

	if(DEVICE_MINOR(dev) == DEVICE_MINOR_ANY) {
		unsigned int minor = 0;

		do {
			dev = DEVICE_MAKE(DEVICE_MAJOR(dev), minor);
			minor++;
		} while(minor != DEVICE_MINOR_ANY && hmgeti(device_map, dev) != -1);

		if(hmgeti(device_map, dev) != -1) return DEVICE_ERROR;
	}

	entry = malloc(sizeof(*entry));
	if(entry == NULL) return DEVICE_ERROR;

	entry->device = device;

	if(name == NULL) {
		snprintf(entry->name, sizeof(entry->name), "%s%u", device->name, device->unit);
	} else {
		if(strlen(name) == 0 || strlen(name) >= sizeof(entry->name)) {
			free(entry);
			return DEVICE_ERROR;
		}

		snprintf(entry->name, sizeof(entry->name), name, DEVICE_MINOR(dev));
	}

	hmput(device_map, dev, entry);

	return dev;
}

void cdev_unregister_device(dev_t dev) {
	struct device_entry* entry = hmget(device_map, dev);

	if(entry != NULL) free(entry);

	hmdel(device_map, dev);
}

struct device* cdev_lookup_device(dev_t dev) {
	struct device_entry* entry = hmget(device_map, dev);

	if(entry == NULL) return NULL;

	return entry->device;
}

int cdev_find_device(const char* name, dev_t* result) {
	unsigned int i;

	for(i = 0; i < hmlen(device_map); i++) {
		if(strcmp(device_map[i].value->name, name) == 0) {
			*result = device_map[i].key;
			return 0;
		}
	}

	return -1;
}

unsigned int cdev_device_count(void) {
	return hmlen(device_map);
}

int cdev_device_at(unsigned int index, dev_t* dev, char* name, size_t name_size) {
	if(index >= hmlen(device_map)) return -1;

	snprintf(name, name_size, "%s", device_map[index].value->name);

	*dev = device_map[index].key;

	return 0;
}

int cdev_first_device(unsigned int major, dev_t* result) {
	unsigned int i;

	for(i = 0; i < hmlen(device_map); i++) {
		if(DEVICE_MAJOR(device_map[i].key) == major) {
			*result = device_map[i].key;
			return 0;
		}
	}

	return -1;
}
