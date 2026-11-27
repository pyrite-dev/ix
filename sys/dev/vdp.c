#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/bus.h>
#include <sys/spinlock.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <sys/tty.h>
#include <sys/ttydev.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/font.h>

#include "vdp.h"

#include <stdlib.h>
#include <string.h>

/*
 * 0x0000 - 0x0800: font
 * 0x0800 - 0x1400: pattern
 * 0x1400 - 0x1480: color
 * 0x1400 - 0x1500: sprite attr
 * 0x1800 - 0x2000: sprite pattern
 */

struct vdp_softc {
	struct tty tty;

	bus_space_t space;

	spinlock_t lock;

	thread_t      thread;
	unsigned char stack[4096];

	int x;
	int y;
};

static void vdp_addr(bus_space_t* space, unsigned short addr, int w) {
	bus_space_write8(space, VDP_ADDR, (addr >> 0) & 0xff);
	bus_space_write8(space, VDP_ADDR, (w ? 0x40 : 0) | ((addr >> 8) & 0xff));
}

static void vdp_putc(struct tty* tty, int c) {
	struct vdp_softc* softc = tty->device->softc;
	intr_flags_t	  flags = spinlock_lock_irqsave(&softc->lock);

	if(c == '\r') {
		softc->x = 0;
	} else if(c == '\n') {
		softc->y++;
	} else {
		vdp_addr(&softc->space, 0x800 + 32 * softc->y + softc->x, 1);
		bus_space_write8(&softc->space, VDP_DATA, c);
		softc->x++;

		if(softc->x == 32) {
			softc->x = 0;
			softc->y++;
		}
	}

	if(softc->y >= 24) {
		int	      line = softc->y - 23;
		unsigned char screen[32 * 24];
		int	      i;

		memset(screen, 0, 32 * 24);

		vdp_addr(&softc->space, 0x800 + 32 * line, 0);
		for(i = 0; i < 32 * (24 - line); i++) screen[i] = bus_space_read8(&softc->space, VDP_DATA);

		vdp_addr(&softc->space, 0x800, 1);
		for(i = 0; i < 32 * 24; i++) {
			bus_space_write8(&softc->space, VDP_DATA, screen[i]);
		}

		softc->y = 23;
	}

	spinlock_unlock_irqrestore(&softc->lock, flags);
}

static const struct tty_ops vdp_tty_ops = {
    NULL,
    NULL,
    NULL,
    vdp_putc};

static int vdp_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static void vdp_thread(void* arg) {
	struct vdp_softc* softc = arg;
	unsigned char	  data;
	int		  i;

	while(1) {
		if(!tty_output_getc(&softc->tty, &data)) continue;

		vdp_putc(&softc->tty, data);
	}
}

static int vdp_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct vdp_softc* softc;
	int		  i;
	int		  j;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	bus_space_init(&softc->space, args->locators);

	spinlock_init(&softc->lock);

	vdp_addr(&softc->space, 0, 1);
	bus_space_write8(&softc->space, VDP_DATA, 0xae);

	vdp_addr(&softc->space, 0, 0);
	if(bus_space_read8(&softc->space, VDP_DATA) != 0xae) {
		free(softc);
		return -1;
	}

	bus_space_write8(&softc->space, VDP_ADDR, 0);
	bus_space_write8(&softc->space, VDP_ADDR, VDP_R0);

	bus_space_write8(&softc->space, VDP_ADDR, VDP_R1_16K | VDP_R1_BLANK);
	bus_space_write8(&softc->space, VDP_ADDR, VDP_R1);

	bus_space_write8(&softc->space, VDP_ADDR, 0x02);
	bus_space_write8(&softc->space, VDP_ADDR, VDP_R2);

	bus_space_write8(&softc->space, VDP_ADDR, 0x50);
	bus_space_write8(&softc->space, VDP_ADDR, VDP_R3);

	bus_space_write8(&softc->space, VDP_ADDR, 0x00);
	bus_space_write8(&softc->space, VDP_ADDR, VDP_R4);

	bus_space_write8(&softc->space, VDP_ADDR, 0x29);
	bus_space_write8(&softc->space, VDP_ADDR, VDP_R5);

	bus_space_write8(&softc->space, VDP_ADDR, 0x03);
	bus_space_write8(&softc->space, VDP_ADDR, VDP_R6);

	bus_space_write8(&softc->space, VDP_ADDR, VDP_COLOR);
	bus_space_write8(&softc->space, VDP_ADDR, VDP_R7);

	vdp_addr(&softc->space, 0, 1);
	for(i = 0; i < 0x4000; i++) bus_space_write8(&softc->space, VDP_DATA, 0);

	vdp_addr(&softc->space, 0, 1);
	for(i = 0; i < 128; i++) {
		for(j = 0; j < 8; j++) {
			int	      k;
			unsigned char g = 0;

			for(k = 0; k < 8; k++) g |= ((font[i][j] >> k) & 1) << (7 - k);

			bus_space_write8(&softc->space, VDP_DATA, g);
		}
	}

	vdp_addr(&softc->space, 0x1400, 1);
	for(i = 0; i < 0x80; i++) bus_space_write8(&softc->space, VDP_DATA, VDP_COLOR);

	if(ttydev_init(self, &softc->tty, NULL, &vdp_tty_ops, 9600) == DEVICE_ERROR) {
		free(softc);
		return -1;
	}

	thread_init(&softc->thread, softc->stack, sizeof(softc->stack), vdp_thread, softc);

	return 0;
}

static void vdp_enable_ints(struct device* self) {
	struct vdp_softc* softc = self->softc;

	sched_add(&softc->thread);
}

static void vdp_detach(struct device* self) {
	struct vdp_softc* softc = self->softc;

	free(softc);
}

struct device_driver vdp_driver = {
    "vdp",
    vdp_match,
    vdp_attach,
    vdp_enable_ints,
    vdp_detach};
