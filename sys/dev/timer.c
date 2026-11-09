#include <sys/device.h>

static int timer_match(struct device* parent, struct device_attach_args* args){
}

static int timer_attach(struct device* parent, struct device* self, struct device_attach_args* args){
}

struct device_driver timer_driver = {
	"timer",
	timer_match,
	timer_attach
};
