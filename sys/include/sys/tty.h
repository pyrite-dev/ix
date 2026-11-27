#ifndef __SYS_TTY_H__
#define __SYS_TTY_H__

struct tty_queue;
struct tty_canrec;
struct tty_canrec_queue;
struct ttv_ops;
struct tty;

#define TTYQ_SIZE 1024

#define TS_ISOPEN (1 << 0)
#define TS_TTSTOP (1 << 1)

#define TTY_FLUSH_INPUT 0x01
#define TTY_FLUSH_OUTPUT 0x02

#include <sys/cdefs.h>
#include <sys/termios.h>
#include <sys/uio.h>
#include <sys/spinlock.h>
#include <sys/waiter.h>
#include <sys/mutex.h>
#include <sys/device.h>

struct tty_queue {
	unsigned char data[TTYQ_SIZE];

	unsigned int head;
	unsigned int tail;
	unsigned int count;
};

struct tty_canrec_queue {
	unsigned int data[TTYQ_SIZE];
	unsigned int head;
	unsigned int tail;
	unsigned int count;
};

struct tty_ops {
	void (*start)(struct tty* tty);
	int (*param)(struct tty* tty, const struct termios* termios);
	void (*stop)(struct tty* tty, int flags);
	void (*putc)(struct tty* tty, int c);
	/* ^^^ MAY be NULL. Called in any context including interrupt handlers and panic.
	   This means, it MUST NOT sleep (no mutex, waiter, or tty calls).
	   It also MUST NOT call kprintf, and MAY only use spinlocks (only ones that would
	   not cause deadlock). */
};

struct tty {
	struct device* device;
	dev_t	       dev;
	struct tty*    next;

	struct tty_queue	rawq;
	struct tty_queue	canq;
	struct tty_canrec_queue canrecq;
	struct tty_queue	outq;

	struct termios termios;

	const struct tty_ops* ops;

	spinlock_t lock;

	waiter_t rwait;
	waiter_t wwait;

	mutex_t read_mutex;
	mutex_t write_mutex;
	mutex_t param_mutex;

	unsigned int state;
	unsigned int open_count;

	unsigned int out_reserved;
};

__BEGIN_EXTERN_C__

#ifdef __KERNEL__
void tty_init(struct tty* tty, const struct tty_ops* ops);

int tty_open(struct tty* tty, int flags, int mode);
int tty_close(struct tty* tty);

int tty_read(struct tty* tty, struct uio* uio, int flags);
int tty_write(struct tty* tty, struct uio* uio, int flags);

int tty_ioctl(struct tty* tty, unsigned long cmd, void* data, int flags);

void tty_start(struct tty* tty);

void tty_input(struct tty* tty, unsigned char data, unsigned int flags);
int  tty_output_getc(struct tty* tty, unsigned char* data);

void tty_flush(struct tty* tty, int flags);

int tty_param(struct tty* tty, const struct termios* termios);
#endif

__END_EXTERN_C__

#endif
