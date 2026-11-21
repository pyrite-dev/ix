#include <sys/cdev.h>
#include <sys/types.h>

void cdev_init(void) {
}

int cdev_register(unsigned int major, const struct cdevsw* cdevsw) {
}

void cdev_unregister(unsigned int major) {
}

const struct cdevsw* cdev_lookup(dev_t dev) {
}
