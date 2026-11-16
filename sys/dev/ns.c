#include <sys/device.h>
#include <sys/interrupt.h>
#include <stddef.h>

static int ns_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static void ns_intr(unsigned int vector, void* arg) {
}

static int ns_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	intr_register(_intr_vector(args->locators[DEVICE_LOCATOR_IRQ]), ns_intr, self);
	return 0;
}

static void ns_detach(struct device* self) {
}

struct device_driver ns_driver = {
    "ns",
    ns_match,
    ns_attach,
    ns_detach};
