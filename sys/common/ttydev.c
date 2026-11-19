#include <sys/ttydev.h>
#include <sys/tty.h>
#include <sys/termios.h>

int ttydev_init(void* softc, const struct ttydev_ops* ops, speed_t baud) {
	struct tty* tty = softc;

	tty_init(tty, ops, softc);

	tty->t_termios.c_ispeed = baud;
	tty->t_termios.c_ospeed = baud;

	return tty_param(tty, &tty->t_termios);
}
