#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/bus.h>
#include <sys/spinlock.h>
#include <sys/tty.h>
#include <sys/errno.h>
#include <sys/cdev.h>

#include "ns.h"

#include <stdlib.h>
#include <string.h>

#define NS_MAX_UNITS 4

struct ns_softc {
	bus_space_t space;

	spinlock_t lock;

	unsigned char ier;
	unsigned char mcr;

	unsigned int clock;
	unsigned int vector;

	struct tty tty;
};

static struct ns_softc* ns_units[NS_MAX_UNITS];

static unsigned short baud_to_div(int clock, int baud) {
	return clock / (16 * baud);
}

static int ns_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static struct ns_softc* ns_lookup(dev_t dev) {
	unsigned int unit;

	unit = MINOR(dev);

	if(unit >= NS_MAX_UNITS) return NULL;

	return ns_units[unit];
}

static int ns_open(dev_t dev, int flags, int mode) {
	struct ns_softc* softc;

	softc = ns_lookup(dev);
	if(softc == NULL) return ENXIO;

	return tty_open(&softc->tty, flags, mode);
}

static int ns_close(dev_t dev, int flags, int mode) {
	struct ns_softc* softc;

	softc = ns_lookup(dev);
	if(softc == NULL) return ENXIO;

	tty_close(&softc->tty);

	return 0;
}

static int ns_read(dev_t dev, struct uio* uio, int flags) {
	struct ns_softc* softc;

	softc = ns_lookup(dev);
	if(softc == NULL) return ENXIO;

	return tty_read(&softc->tty, uio, flags);
}

static int ns_write(dev_t dev, struct uio* uio, int flags) {
	struct ns_softc* softc;

	softc = ns_lookup(dev);
	if(softc == NULL) return ENXIO;

	return tty_write(&softc->tty, uio, flags);
}

static int ns_ioctl(dev_t dev, unsigned long cmd, void* data, int flags) {
	struct ns_softc* softc;

	softc = ns_lookup(dev);
	if(softc == NULL) return ENXIO;

	return tty_ioctl(&softc->tty, cmd, data, flags);
}

struct cdevsw ns_cdevsw = {
    ns_open,
    ns_close,
    ns_read,
    ns_write,
    ns_ioctl};

static void ns_start(struct tty* tty) {
	struct ns_softc* softc;
	intr_flags_t	 flags;

	softc = tty->t_devdata;

	flags = spinlock_lock_irqsave(&softc->lock);

	softc->ier |= NS_IE_THRE;
	bus_space_write8(&softc->space, NS_IER, softc->ier);

	spinlock_unlock_irqrestore(&softc->lock, flags);
}

static int ns_param(struct tty* tty, const struct termios* termios) {
	struct ns_softc* softc;
	unsigned long	 baud;
	unsigned long	 div;
	unsigned char	 lcr;
	intr_flags_t	 flags;

	softc = tty->t_devdata;

	baud = termios->c_ospeed;

	if(baud == B0) {
		flags = spinlock_lock_irqsave(&softc->lock);

		softc->mcr &= ~NS_MC_DTR;
		bus_space_write8(&softc->space, NS_MCR, softc->mcr);

		spinlock_unlock_irqrestore(&softc->lock, flags);
		return 0;
	}

	if((termios->c_cflag & CSIZE) != CS8) return EINVAL;

	if(termios->c_cflag & (CSTOPB | PARENB)) return EINVAL;

	if(termios->c_ispeed != 0 && termios->c_ispeed != termios->c_ospeed) return EINVAL;

	div = baud_to_div(softc->clock, baud);

	if(div == 0 || div > 65535) return EINVAL;

	lcr = NS_LC_PB_N | NS_LC_SB | NS_LC_DB_8;

	flags = spinlock_lock_irqsave(&softc->lock);

	bus_space_write8(&softc->space, NS_IER, 0);

	bus_space_write8(&softc->space, NS_LCR, NS_LC_DLAB);
	bus_space_write8(&softc->space, NS_DHR, (div >> 8) & 0xff);
	bus_space_write8(&softc->space, NS_DLR, (div >> 0) & 0xff);
	bus_space_write8(&softc->space, NS_LCR, lcr);

	softc->mcr |= NS_MC_DTR;
	bus_space_write8(&softc->space, NS_MCR, softc->mcr);

	bus_space_write8(&softc->space, NS_IER, softc->ier);

	spinlock_unlock_irqrestore(&softc->lock, flags);

	return 0;
}

static const struct ttydev_ops ns_tty_ops = {
    ns_start,
    ns_param,
    NULL};

static void ns_rx_intr(struct ns_softc* softc) {
	while(1) {
		intr_flags_t  flags = spinlock_lock_irqsave(&softc->lock);
		unsigned char lsr;
		unsigned char data;

		if(!((lsr = bus_space_read8(&softc->space, NS_LSR)) & NS_LS_DR)){
			spinlock_unlock_irqrestore(&softc->lock, flags);
			break;
		}

		data = bus_space_read8(&softc->space, NS_DR);

		spinlock_unlock_irqrestore(&softc->lock, flags);

		tty_input(&softc->tty, data, 0);
	}
}

static void ns_tx_intr(struct ns_softc* softc) {
	intr_flags_t flags = spinlock_lock_irqsave(&softc->lock);
	int	     i;

	/* TODO: Get FIFO size */
	for(i = 0; i < 1; i++) {
		unsigned char data;

		if(!tty_output_getc(&softc->tty, &data)) break;

		bus_space_write8(&softc->space, NS_DR, data);
	}

	if(i == 0) {
		softc->ier &= ~NS_IE_THRE;
		bus_space_write8(&softc->space, NS_IER, softc->ier);
	}

	spinlock_unlock_irqrestore(&softc->lock, flags);
}

static void ns_intr(unsigned int vector, struct iregs* iregs, void* arg) {
	struct ns_softc* softc = ((struct device*)arg)->softc;
	unsigned char	 iir;

	while(((iir = bus_space_read8(&softc->space, NS_IIR)) & NS_II_IP) == 0) {
		switch(iir & NS_II_IS) {
		case NS_II_IS_THRE:
			ns_tx_intr(softc);
			break;
		case NS_II_IS_RDA:
			ns_rx_intr(softc);
			break;
		}
	}
}

static int ns_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	unsigned int	 clock = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_CLOCK], 115200);
	unsigned int	 baud  = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_BAUD], 9600);
	unsigned short	 div   = baud_to_div(clock, baud);
	struct ns_softc* softc;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	bus_space_init(&softc->space, args->locators);

	spinlock_init(&softc->lock);

	softc->clock = clock;

	bus_space_write8(&softc->space, NS_IER, 0);
	bus_space_write8(&softc->space, NS_LCR, NS_LC_DLAB);
	bus_space_write8(&softc->space, NS_DHR, (div >> 8) & 0xff);
	bus_space_write8(&softc->space, NS_DLR, (div >> 0) & 0xff);
	bus_space_write8(&softc->space, NS_LCR, NS_LC_PB_N | NS_LC_SB | NS_LC_DB_8);
	bus_space_write8(&softc->space, NS_IIR, NS_FC_ITL_1 | NS_FC_CTFIFO | NS_FC_CRFIFO | NS_FC_EFIFO);
	bus_space_write8(&softc->space, NS_MCR, NS_MC_L | NS_MC_O2 | NS_MC_O1 | NS_MC_RTS);

	while(!(bus_space_read8(&softc->space, NS_LSR) & NS_LS_THRE));
	bus_space_write8(&softc->space, NS_DR, 0xae);

	while(!(bus_space_read8(&softc->space, NS_LSR) & NS_LS_DR));
	if(bus_space_read8(&softc->space, NS_DR) != 0xae) {
		/* serial is faulty */
		free(softc);
		return -1;
	}

	softc->ier = NS_IE_RDA;
	softc->mcr = NS_MC_O2 | NS_MC_O1 | NS_MC_RTS | NS_MC_DTR;

	bus_space_write8(&softc->space, NS_MCR, softc->mcr);

	tty_init(&softc->tty, &ns_tty_ops, softc);

	if(self->unit >= NS_MAX_UNITS){
		free(softc);
		return -1;
	}
	ns_units[self->unit] = softc;

	softc->tty.t_termios.c_ispeed = baud;
	softc->tty.t_termios.c_ospeed = baud;

	if(tty_param(&softc->tty, &softc->tty.t_termios) != 0){
		free(softc);
		return -1;
	}

	softc->vector = _intr_vector(args->locators[DEVICE_LOCATOR_IRQ]);

	intr_register(softc->vector, ns_intr, self);
	intr_enable(softc->vector);

	bus_space_write8(&softc->space, NS_IER, softc->ier);
	return 0;
}

static void ns_detach(struct device* self) {
	struct ns_softc* softc = self->softc;

	intr_disable(softc->vector);

	bus_space_write8(&softc->space, NS_IER, 0);

	intr_unregister(softc->vector);

	free(softc);
}

struct device_driver ns_driver = {
    "ns",
    ns_match,
    ns_attach,
    ns_detach};
