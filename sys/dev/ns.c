#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/bus.h>
#include <sys/spinlock.h>
#include <sys/tty.h>
#include <sys/ttydev.h>
#include <sys/errno.h>
#include <sys/time.h>

#include "ns.h"

#include <stdlib.h>
#include <string.h>

struct ns_softc {
	struct tty tty;

	bus_space_t space;

	spinlock_t lock;

	unsigned char ier;
	unsigned char mcr;

	unsigned int clock;
	unsigned int vector;
};

static unsigned int baud_to_div(int clock, int baud) {
	return clock / (16 * baud);
}

static int ns_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

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

		if(!((lsr = bus_space_read8(&softc->space, NS_LSR)) & NS_LS_DR)) {
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
	unsigned int	 div;
	struct ns_softc* softc;
	unsigned long	 limit = ticktime(100);
	unsigned long	 start;
	int		 i;

	if(baud == 0) {
		return -1;
	}

	div = baud_to_div(clock, baud);

	if(div == 0 || div > 65535) {
		return -1;
	}

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
	bus_space_write8(&softc->space, NS_FCR, NS_FC_ITL_1 | NS_FC_CTFIFO | NS_FC_CRFIFO | NS_FC_EFIFO);
	bus_space_write8(&softc->space, NS_MCR, NS_MC_L | NS_MC_O2 | NS_MC_O1 | NS_MC_RTS);

	for(i = 0; i < 64; i++) {
		if(!bus_space_read8(&softc->space, NS_LSR) & NS_LS_DR) break;

		bus_space_read8(&softc->space, NS_DR);
	}

	start = ticks;
	while(!(bus_space_read8(&softc->space, NS_LSR) & NS_LS_THRE)) {
		if(ticks - start >= limit) {
			/* serial is faulty */
			bus_space_write8(&softc->space, NS_MCR, NS_MC_O2 | NS_MC_O1 | NS_MC_RTS | NS_MC_DTR);
			free(softc);
			return -1;
		}
	}

	bus_space_write8(&softc->space, NS_DR, 0xae);

	start = ticks;
	while(!(bus_space_read8(&softc->space, NS_LSR) & NS_LS_DR)) {
		if(ticks - start >= limit) {
			/* serial is faulty */
			bus_space_write8(&softc->space, NS_MCR, NS_MC_O2 | NS_MC_O1 | NS_MC_RTS | NS_MC_DTR);
			free(softc);
			return -1;
		}
	}

	if(bus_space_read8(&softc->space, NS_DR) != 0xae) {
		/* serial is faulty */
		bus_space_write8(&softc->space, NS_MCR, NS_MC_O2 | NS_MC_O1 | NS_MC_RTS | NS_MC_DTR);
		free(softc);
		return -1;
	}

	softc->ier = 0;
	softc->mcr = NS_MC_O2 | NS_MC_O1 | NS_MC_RTS | NS_MC_DTR;

	bus_space_write8(&softc->space, NS_MCR, softc->mcr);

	if(ttydev_init(self, &ns_tty_ops, baud) != 0) {
		free(softc);
		return -1;
	}

	softc->vector = _intr_vector(args->locators[DEVICE_LOCATOR_IRQ]);
	return 0;
}

static void ns_enable_ints(struct device* self) {
	struct ns_softc* softc = self->softc;

	intr_register(softc->vector, ns_intr, self);
	intr_enable(softc->vector);

	softc->ier = NS_IE_RDA;
	bus_space_write8(&softc->space, NS_IER, softc->ier);
}

static void ns_detach(struct device* self) {
	struct ns_softc* softc = self->softc;

	intr_disable(softc->vector);

	softc->ier = 0;
	bus_space_write8(&softc->space, NS_IER, softc->ier);

	intr_unregister(softc->vector);

	free(softc);
}

struct device_driver ns_driver = {
    "ns",
    ns_match,
    ns_attach,
    ns_enable_ints,
    ns_detach};
