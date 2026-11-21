#include <sys/ttydev.h>
#include <sys/tty.h>
#include <sys/termios.h>
#include <sys/errno.h>
#include <sys/cdev.h>
#include <sys/device.h>
#include <sys/major.h>

static struct tty* ttydev_lookup(dev_t dev) {
	struct device* device;

	device = cdev_lookup_device(dev);
	if(device == NULL) return NULL;

	return (struct tty*)device->softc;
}

static int ttydev_open(dev_t dev, int flags, int mode) {
	struct tty* tty;

	tty = ttydev_lookup(dev);
	if(tty == NULL) return ENXIO;

	return tty_open(tty, flags, mode);
}

static int ttydev_close(dev_t dev, int flags, int mode) {
	struct tty* tty;

	tty = ttydev_lookup(dev);
	if(tty == NULL) return ENXIO;

	tty_close(tty);

	return 0;
}

static int ttydev_read(dev_t dev, struct uio* uio, int flags) {
	struct tty* tty;

	tty = ttydev_lookup(dev);
	if(tty == NULL) return ENXIO;

	return tty_read(tty, uio, flags);
}

static int ttydev_write(dev_t dev, struct uio* uio, int flags) {
	struct tty* tty;

	tty = ttydev_lookup(dev);
	if(tty == NULL) return ENXIO;

	return tty_write(tty, uio, flags);
}

static int ttydev_ioctl(dev_t dev, unsigned long cmd, void* data, int flags) {
	struct tty* tty;

	tty = ttydev_lookup(dev);
	if(tty == NULL) return ENXIO;

	return tty_ioctl(tty, cmd, data, flags);
}

struct cdevsw tty_cdevsw = {
    ttydev_open,
    ttydev_close,
    ttydev_read,
    ttydev_write,
    ttydev_ioctl};

int ttydev_init(struct device* device, const struct ttydev_ops* ops, speed_t baud) {
	struct tty* tty = device->softc;
	int	    st;

	tty_init(tty, ops, device->softc);

	tty->t_termios.c_ispeed = baud;
	tty->t_termios.c_ospeed = baud;

	if((st = tty_param(tty, &tty->t_termios)) != 0) return st;
	if(cdev_register_device(DEVICE_MAKE(CDEV_MAJOR_TTY, DEVICE_MINOR_ANY), device) == DEVICE_ERROR) return -1;

	return 0;
}
