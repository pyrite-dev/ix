#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/dev/ns.h>
#include <sys/bus.h>
#include <sys/spinlock.h>
#include <stdlib.h>
#include <string.h>

struct ns_ring {
	unsigned char data[NS_BUFSZ];
	unsigned int  head;
	unsigned int  tail;
};

struct ns_softc {
	bus_space_t space;

	spinlock_t lock;

	unsigned char ier;

	struct ns_ring rx;
	struct ns_ring tx;
};

static unsigned short baud_to_div(int baud) {
	return 10000000 / (16 * baud);
}

static int ns_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static void ns_rx_intr(struct ns_softc* softc) {
	intr_flags_t flags = spinlock_lock_irqsave(&softc->lock);
	while(1) {
		unsigned char lsr;
		unsigned char data;
		unsigned int  next;

		if(!((lsr = bus_space_read8(&softc->space, NS_LSR)) & NS_LS_DR)) break;

		data = bus_space_read8(&softc->space, NS_DR);
		next = (softc->rx.head + 1) % NS_BUFSZ;

		if(next == softc->rx.tail) {
			/* overrun */
			continue;
		}

		softc->rx.data[softc->rx.head] = data;
		softc->rx.head		       = next;
	}
	spinlock_unlock_irqrestore(&softc->lock, flags);

	/* wake up writer */
}

static void ns_tx_intr(struct ns_softc* softc) {
	intr_flags_t flags = spinlock_lock_irqsave(&softc->lock);
	int	     i;

	/* TODO: Get FIFO size */
	for(i = 0; i < 1; i++) {
		if(softc->tx.head == softc->tx.tail) break;

		bus_space_write8(&softc->space, NS_DR, softc->tx.data[softc->tx.tail]);
		softc->tx.tail = (softc->tx.tail + 1) % NS_BUFSZ;
	}

	if(softc->tx.head == softc->tx.tail) {
		softc->ier &= ~NS_IE_THRE;
		bus_space_write8(&softc->space, NS_IER, softc->ier);
	}

	spinlock_unlock_irqrestore(&softc->lock, flags);

	/* wake up reader */
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
	unsigned short	 div = baud_to_div(9600);
	struct ns_softc* softc;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	bus_space_init(&softc->space, args->locators);

	spinlock_init(&softc->lock);

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
		free(self->softc);
		return -1;
	}

	softc->ier = NS_IE_RDA;

	bus_space_write8(&softc->space, NS_MCR, NS_MC_O2 | NS_MC_O1 | NS_MC_RTS | NS_MC_DTR);

	intr_register(_intr_vector(args->locators[DEVICE_LOCATOR_IRQ]), ns_intr, self);
	intr_enable(_intr_vector(args->locators[DEVICE_LOCATOR_IRQ]));

	bus_space_write8(&softc->space, NS_IER, softc->ier);
	return 0;
}

static void ns_detach(struct device* self) {
	free(self->softc);
}

struct device_driver ns_driver = {
    "ns",
    ns_match,
    ns_attach,
    ns_detach};
