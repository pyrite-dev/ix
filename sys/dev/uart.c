#include <sys/device.h>

static int uart_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int uart_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	return 0;
}

static void uart_detach(struct device* self) {
}

struct device_driver uart_driver = {
    "uart",
    uart_match,
    uart_attach,
    uart_detach};
