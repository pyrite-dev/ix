#include <sys/device.h>

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

static struct device* root;
static const char*    locators[] = DEVICE_LOCATORS;

static struct device* device_new(const char* name, int unit) {
	struct device* dev = malloc(sizeof(*dev));

	if(dev == NULL) return NULL;

	if((dev->name = strdup(name)) == NULL) {
		free(dev);
		return NULL;
	}
	dev->unit   = unit;
	dev->driver = NULL;

	dev->parent	 = NULL;
	dev->prev	 = NULL;
	dev->next	 = NULL;
	dev->first_child = NULL;

	return dev;
}

static void device_link(struct device* parent, struct device* dev) {
	dev->parent = parent;

	if(parent->first_child == NULL) {
		parent->first_child = dev;
	} else {
		struct device* child = parent->first_child;

		while(child->next != NULL) child = child->next;
		child->next = dev;
		dev->prev   = child;
	}
}

static void device_remove(struct device* dev) {
	struct device* child;

	while(dev->first_child != NULL) {
		device_remove(dev->first_child);
	}

	if(dev->driver != NULL && dev->driver->detach != NULL) {
		dev->driver->detach(dev);
	}

	if(dev->parent != NULL) {
		if(dev->prev == NULL) {
			dev->parent->first_child = dev->next;
		} else {
			dev->prev->next = dev->next;
		}

		if(dev->next != NULL) {
			dev->next->prev = dev->prev;
		}
	}

	free(dev->name);
	free(dev);
}

static int device_unit_used(struct device* parent, const char* dev, int unit) {
	struct device* child = parent->first_child;

	while(child != NULL) {
		if(strcmp(child->name, dev) == 0 && child->unit == unit) return 1;

		if(device_unit_used(child, dev, unit)) return 1;

		child = child->next;
	}

	return 0;
}

static int device_alloc_unit(const char* name) {
	int unit;

	for(unit = 0;; unit++) {
		if(!device_unit_used(root, name, unit)) return unit;
	}

	return 0;
}

static int device_parent_match(struct device* parent, struct device_config* config) {
	if(strcmp(parent->name, config->parent) != 0) return 0;

	if(config->parent_unit != DEVICE_UNIT_ANY && parent->unit != config->parent_unit) return 0;

	return 1;
}

static int device_attach(struct device* parent, struct device_config* config) {
	struct device_attach_args args;
	struct device_driver*	  driver;
	struct device*		  dev;
	int			  unit;
	int			  i;

	driver = config->driver;

	memcpy(args.locators, config->locators, sizeof(args.locators));
	if(!driver->match(parent, &args)) return 0;

	if(config->unit == DEVICE_UNIT_ANY) {
		unit = device_alloc_unit(config->name);
	} else {
		unit = config->unit;
	}

	if((dev = device_new(config->name, unit)) == NULL) {
		return 0;
	}
	dev->driver = driver;

	if(driver->attach(parent, dev, &args) != 0) {
		free(dev->name);
		free(dev);
		return 0;
	}

	kprintf("%s%d at %s%d", dev->name, dev->unit, parent->name, parent->unit);
	for(i = 0; i < DEVICE_LOCATOR_COUNT; i++) {
		if(args.locators[i] == DEVICE_LOCATOR_UNSET) continue;
		kprintf(" %s ", locators[i * 2 + 0]);
		kprintf(locators[i * 2 + 1], args.locators[i]);
	}
	kprintf("\r\n");

	device_link(parent, dev);

	return 1;
}

static int device_configure(struct device* node, struct device_config* config) {
	struct device* child;
	int	       count = 0;

	if(device_parent_match(node, config)) {
		if(device_attach(node, config)) count++;

		if(config->unit != DEVICE_UNIT_ANY && count > 0) return count;
	}

	child = node->first_child;
	while(child != NULL) {
		struct device* next = child->next;

		count += device_configure(child, config);

		child = next;

		if(config->unit != DEVICE_UNIT_ANY && count > 0) break;
	}

	return count;
}

void device_init(void) {
	int i;

	root = device_new("root", 0); /* root needs to be unit 0 as config.pl takes missing unit number as 0 */

	for(i = 0; devices[i].name != NULL; i++) {
		device_configure(root, &devices[i]);
	}
}
