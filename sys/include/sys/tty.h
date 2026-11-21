#ifndef __SYS_TTY_H__
#define __SYS_TTY_H__

struct tty_queue;
struct tty;

#define TTYQ_SIZE 1024

#define TS_ISOPEN (1 << 0)
#define TS_TTSTOP (1 << 1)

#define TTY_FLUSH_INPUT 0x01
#define TTY_FLUSH_OUTPUT 0x02

#include <sys/cdefs.h>
#include <sys/termios.h>
#include <sys/ttydev.h>
#include <sys/uio.h>
#include <sys/spinlock.h>
#include <sys/waiter.h>
#include <sys/mutex.h>

struct tty_queue {
	unsigned char data[TTYQ_SIZE];

	unsigned int head;
	unsigned int tail;
	unsigned int count;
};

struct tty {
	struct tty_queue t_rawq;
	struct tty_queue t_canq;
	struct tty_queue t_outq;

	struct termios t_termios;

	const struct ttydev_ops* t_devops;
	void*			 t_devdata;

	spinlock_t t_lock;

	waiter_t t_rwait;
	waiter_t t_wwait;

	mutex_t t_read_mutex;
	mutex_t t_write_mutex;
	mutex_t t_param_mutex;

	unsigned int t_state;
	unsigned int t_open_count;

	unsigned int t_out_reserved;
};

__BEGIN_EXTERN_C__

void tty_init(struct tty* tty, const struct ttydev_ops* ops, void* devdata);

int  tty_open(struct tty* tty, int flags, int mode);
void tty_close(struct tty* tty);

int tty_read(struct tty* tty, struct uio* uio, int flags);
int tty_write(struct tty* tty, struct uio* uio, int flags);

int tty_ioctl(struct tty* tty, unsigned long cmd, void* data, int flags);

void tty_start(struct tty* tty);

void tty_input(struct tty* tty, unsigned char data, unsigned int flags);
int  tty_output_getc(struct tty* tty, unsigned char* data);

void tty_flush(struct tty* tty, int flags);

int tty_param(struct tty* tty, const struct termios* termios);

__END_EXTERN_C__

#endif
