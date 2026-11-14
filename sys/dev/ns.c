#include <sys/device.h>

static int ns_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int ns_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	return 0;
}

static void ns_detach(struct device* self) {
}

struct device_driver ns_driver = {
    "ns",
    ns_match,
    ns_attach,
    ns_detach};
