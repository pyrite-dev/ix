#include <sys/ttydev.h>
#include <sys/tty.h>
#include <sys/termios.h>
#include <sys/errno.h>
#include <sys/cdev.h>
#include <sys/cons.h>
#include <sys/msgbuf.h>
#include <sys/device.h>
#include <sys/major.h>

static struct tty* ttydev_cons_tty;

static void ttydev_cons_putc(int c) {
	ttydev_cons_tty->ops->putc(ttydev_cons_tty, c);
}

static struct tty* ttydev_lookup(dev_t dev) {
	struct device* device;
	struct tty*    tty;

	device = cdev_lookup_device(dev);
	if(device == NULL) return NULL;

	for(tty = device->softc; tty != NULL; tty = tty->next) {
		if(tty->dev == dev) return tty;
	}

	return NULL;
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

	return tty_close(tty);
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

dev_t ttydev_init(struct device* device, struct tty* tty, const char* name, const struct tty_ops* ops, speed_t baud) {
	struct tty* anchor = device->softc;
	int	    st;

	tty_init(tty, ops);

	tty->device = device;

	tty->termios.c_ispeed = baud;
	tty->termios.c_ospeed = baud;

	if((st = tty_param(tty, &tty->termios)) != 0) return st;
	if((tty->dev = cdev_register_device(DEVICE_MAKE(CDEV_MAJOR_TTY, DEVICE_MINOR_ANY), device, name == NULL ? "tty%d" : name)) == DEVICE_ERROR) return DEVICE_ERROR;

	if(DEVICE_MINOR(tty->dev) == 0 && ops->putc != NULL) {
		unsigned int pos = 0;
		int	     c;

		ttydev_cons_tty = tty;
		cons_output_register(ttydev_cons_putc);

		while((c = msgbuf_peek(&pos)) >= 0) cons_putc(c);
	}

	if(tty != anchor) {
		tty->next    = anchor->next;
		anchor->next = tty;
	}

	return tty->dev;
}

void ttydev_detach(struct device* device) {
	struct tty* tty;

	for(tty = (struct tty*)device->softc; tty != NULL; tty = tty->next) {
		cdev_unregister_device(tty->dev);
	}
}
