#include <sys/tty.h>
#include <sys/ttydev.h>
#include <sys/termios.h>

void tty_init(struct tty* tty, const struct ttydev_ops* ops, void* devdata) {
}

int tty_open(struct tty* tty, int flags, int mode) {
}

void tty_close(struct tty* tty) {
}

int tty_read(struct tty* tty, struct uio* uio, int flags) {
}

int tty_write(struct tty* tty, struct uio* uio, int flags) {
}

int tty_ioctl(struct tty* tty, unsigned long cmd, void* data, int flags) {
}

void tty_start(struct tty* tty) {
}

void tty_input(struct tty* tty, unsigned char data, unsigned int flags) {
}

int tty_output_getc(struct tty* tty, unsigned char* data) {
}

void tty_flush(struct tty* tty, int flags) {
}

int tty_param(struct tty* tty, const struct termios* termios) {
}
