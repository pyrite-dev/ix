#include <sys/device.h>

static int pit_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int pit_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	return 0;
}

static void pit_detach(struct device* self) {
}

struct device_driver pit_driver = {
    "pit",
    pit_match,
    pit_attach,
    pit_detach};
