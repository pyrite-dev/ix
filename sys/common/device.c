#include <sys/device.h>
#include <sys/kmem.h>
#include <stddef.h>
#include <string.h>

struct device* root;

static struct device* device_new(struct device* parent, const char* name) {
	struct device* dev = kmalloc(sizeof(*dev));

	dev->name = strdup(name);

	dev->parent = parent;
	dev->prev   = NULL;
	dev->next   = NULL;

	if(parent != NULL) {
		if(parent->first_child == NULL) {
			parent->first_child = dev;
		} else {
			struct device* child = parent->first_child;

			while(child->next != NULL) child = child->next;

			child->next = dev;
			dev->prev   = child;
		}
	}

	return dev;
}

static struct device* device_find(struct device* parent, const char* dev) {
	struct device* child = parent->first_child;

	while(child != NULL) {
		if(strcmp(child->name, dev) == 0) return child;

		child = child->next;
	}

	return NULL;
}

void device_init(void) {
	int			  i;
	struct device_attach_args args;

	root = device_new(NULL, "root");

	for(i = 0; devices[i].name != NULL; i++) {
		memcpy(args.locators, devices[i].locators, sizeof(args.locators));
	}
}
