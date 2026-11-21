#include <sys/tty.h>
#include <sys/ttydev.h>
#include <sys/termios.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

static int ttyq_put(struct tty_queue* q, unsigned char data) {
	if(q->count >= TTYQ_SIZE) return 0;

	q->data[q->tail] = data;
	q->tail		 = (q->tail + 1) % TTYQ_SIZE;
	q->count++;

	return 1;
}

static int ttyq_get(struct tty_queue* q, unsigned char* data) {
	if(q->count == 0) return 0;

	*data	= q->data[q->head];
	q->head = (q->head + 1) % TTYQ_SIZE;
	q->count--;

	return 1;
}

static int ttyq_unput(struct tty_queue* q) {
	if(q->count == 0) return 0;

	q->tail = (q->tail + TTYQ_SIZE - 1) % TTYQ_SIZE;
	q->count--;

	return 1;
}

static void ttyq_clear(struct tty_queue* q) {
	q->head	 = 0;
	q->tail	 = 0;
	q->count = 0;
}

static unsigned int ttyq_space(struct tty_queue* q) {
	return TTYQ_SIZE - q->count;
}

void tty_init(struct tty* tty, const struct ttydev_ops* ops, void* devdata) {
	memset(tty, 0, sizeof(*tty));

	spinlock_init(&tty->t_lock);

	waiter_init(&tty->t_rwait);
	waiter_init(&tty->t_wwait);

	mutex_init(&tty->t_read_mutex);
	mutex_init(&tty->t_write_mutex);
	mutex_init(&tty->t_param_mutex);

	tty->t_devops  = ops;
	tty->t_devdata = devdata;

	tty->t_termios.c_iflag = ICRNL;
	tty->t_termios.c_oflag = OPOST | ONLCR;
	tty->t_termios.c_cflag = CREAD | CS8;
	tty->t_termios.c_lflag = ICANON | ECHO | ECHOE;

	tty->t_termios.c_cc[VERASE] = 0x7f;
	tty->t_termios.c_cc[VKILL]  = 0x15;
	tty->t_termios.c_cc[VEOF]   = 0x04;
	tty->t_termios.c_cc[VSTART] = 0x11;
	tty->t_termios.c_cc[VSTOP]  = 0x13;
	tty->t_termios.c_cc[VMIN]   = 1;
	tty->t_termios.c_cc[VTIME]  = 0;
}

int tty_open(struct tty* tty, int flags, int mode) {
	intr_flags_t irq;

	(void)flags;
	(void)mode;

	irq = spinlock_lock_irqsave(&tty->t_lock);

	if(tty->t_open_count) tty->t_state |= TS_ISOPEN;

	tty->t_open_count++;
	spinlock_unlock_irqrestore(&tty->t_lock, irq);

	return 0;
}

void tty_close(struct tty* tty) {
	intr_flags_t irq;

	irq = spinlock_lock_irqsave(&tty->t_lock);

	if(tty->t_open_count != 0) {
		tty->t_open_count--;

		if(tty->t_open_count == 0) tty->t_state &= ~TS_ISOPEN;
	}

	spinlock_unlock_irqrestore(&tty->t_lock, irq);
}

int tty_read(struct tty* tty, struct uio* uio, int flags) {
	intr_flags_t	  irq;
	struct tty_queue* q;
	unsigned char	  c;
	size_t		  initial, limit, minimum;
	int		  st, canonical, vmin, vtime;

	st	= 0;
	initial = uio->uio_resid;

	mutex_lock(&tty->t_read_mutex);

	while(uio->uio_resid != 0) {
		irq = spinlock_lock_irqsave(&tty->t_lock);

		canonical = (tty->t_termios.c_lflag & ICANON) != 0;

		if(canonical) {
			q	= &tty->t_canq;
			minimum = 1;
		} else {
			q = &tty->t_rawq;

			vmin  = tty->t_termios.c_cc[VMIN];
			vtime = tty->t_termios.c_cc[VTIME];

			if(vtime != 0) {
				spinlock_unlock_irqrestore(&tty->t_lock, irq);
				st = EOPNOTSUPP;
				break;
			}

			minimum = (size_t)vmin;
			if(minimum > uio->uio_resid) minimum = uio->uio_resid;
		}

		while(q->count < minimum) {
			if((flags & O_NONBLOCK) && q->count == 0) {
				spinlock_unlock_irqrestore(&tty->t_lock, irq);
				st = EWOULDBLOCK;
				goto done;
			}

			if((flags & O_NONBLOCK) || minimum == 0) break;

			waiter_sleep(&tty->t_rwait, &tty->t_lock, irq);

			canonical = (tty->t_termios.c_lflag & ICANON) != 0;
			q	  = canonical ? &tty->t_canq : &tty->t_rawq;

			if(!canonical) {
				vmin	= tty->t_termios.c_cc[VMIN];
				minimum = (size_t)vmin;

				if(minimum > uio->uio_resid) minimum = uio->uio_resid;
			} else {
				minimum = 1;
			}
		}

		limit = q->count;
		if(limit > uio->uio_resid) limit = uio->uio_resid;

		spinlock_unlock_irqrestore(&tty->t_lock, irq);

		while(limit != 0 && uio->uio_resid != 0) {
			irq = spinlock_lock_irqsave(&tty->t_lock);

			if(!ttyq_get(q, &c)) {
				spinlock_unlock_irqrestore(&tty->t_lock, irq);
				break;
			}

			spinlock_unlock_irqrestore(&tty->t_lock, irq);

			st = uiomove(&c, 1, uio);
			if(st != 0) goto done;

			limit--;

			if(canonical && c == '\n') break;
		}

		if(canonical) break;

		break;
	}

done:
	mutex_unlock(&tty->t_read_mutex);

	if(st != 0 && initial != uio->uio_resid) st = 0;

	return st;
}

int tty_write(struct tty* tty, struct uio* uio, int flags) {
	intr_flags_t  irq;
	unsigned char c;
	unsigned int  oflag;
	size_t	      initial;
	int	      st;

	st	= 0;
	initial = uio->uio_resid;

	mutex_lock(&tty->t_write_mutex);

	while(uio->uio_resid != 0) {
		irq = spinlock_lock_irqsave(&tty->t_lock);

		while(ttyq_space(&tty->t_outq) < tty->t_out_reserved + 2) {
			if(flags & O_NONBLOCK) {
				spinlock_unlock_irqrestore(&tty->t_lock, irq);
				st = EWOULDBLOCK;
				goto done;
			}

			waiter_sleep(&tty->t_wwait, &tty->t_lock, irq);
		}

		tty->t_out_reserved += 2;

		spinlock_unlock_irqrestore(&tty->t_lock, irq);

		st = uiomove(&c, 1, uio);

		irq = spinlock_lock_irqsave(&tty->t_lock);

		tty->t_out_reserved -= 2;

		if(st == 0) {
			oflag = tty->t_termios.c_oflag;

			if((oflag & OPOST) && (oflag & ONLCR) && c == '\n') {
				ttyq_put(&tty->t_outq, '\r');
				ttyq_put(&tty->t_outq, '\n');
			} else {
				ttyq_put(&tty->t_outq, c);
			}
		}

		spinlock_unlock_irqrestore(&tty->t_lock, irq);

		if(st != 0) break;

		tty_start(tty);
	}

done:
	mutex_unlock(&tty->t_write_mutex);

	if(st != 0 && initial != uio->uio_resid) st = 0;

	return st;
}

static void tty_drain(struct tty* tty) {
	intr_flags_t irq;

	mutex_lock(&tty->t_write_mutex);

	irq = spinlock_lock_irqsave(&tty->t_lock);

	while(tty->t_outq.count != 0) waiter_sleep(&tty->t_wwait, &tty->t_lock, irq);

	spinlock_unlock_irqrestore(&tty->t_lock, irq);

	mutex_unlock(&tty->t_write_mutex);
}

int tty_ioctl(struct tty* tty, unsigned long cmd, void* data, int flags) {
	intr_flags_t irq;

	(void)flags;

	switch(cmd) {
	case TIOCGETA:
		irq		       = spinlock_lock_irqsave(&tty->t_lock);
		*(struct termios*)data = tty->t_termios;
		spinlock_unlock_irqrestore(&tty->t_lock, irq);
		return 0;

	case TIOCSETA:
		return tty_param(tty, (const struct termios*)data);

	case TIOCSETAW:
		tty_drain(tty);
		return tty_param(tty, (const struct termios*)data);

	case TIOCSETAF:
		tty_flush(tty, TTY_FLUSH_INPUT);
		return tty_param(tty, (const struct termios*)data);
	}

	return ENOTTY;
}

void tty_start(struct tty* tty) {
	intr_flags_t		 irq;
	const struct ttydev_ops* ops;
	int			 start;

	irq = spinlock_lock_irqsave(&tty->t_lock);

	start = (tty->t_outq.count != 0) && !(tty->t_state & TS_TTSTOP);

	ops = tty->t_devops;

	spinlock_unlock_irqrestore(&tty->t_lock, irq);

	if(start && ops != NULL && ops->start != NULL) ops->start(tty);
}

static int tty_echo_locked(struct tty* tty, unsigned char c) {
	unsigned int oflag;

	oflag = tty->t_termios.c_oflag;

	if((oflag & OPOST) && (oflag & ONLCR) && c == '\n') {
		if(ttyq_space(&tty->t_outq) <= tty->t_out_reserved + 1) return 0;

		ttyq_put(&tty->t_outq, '\r');
	} else if(ttyq_space(&tty->t_outq) <= tty->t_out_reserved) {
		return 0;
	}

	return ttyq_put(&tty->t_outq, c);
}

void tty_input(struct tty* tty, unsigned char data, unsigned int flags) {
	intr_flags_t  irq;
	unsigned int  iflag, lflag;
	unsigned char c;
	int	      wakeup, start, canonical, delimiter;

	(void)flags;

	c      = data;
	wakeup = 0;
	start  = 0;

	irq = spinlock_lock_irqsave(&tty->t_lock);

	iflag	  = tty->t_termios.c_iflag;
	lflag	  = tty->t_termios.c_lflag;
	canonical = (lflag & ICANON) != 0;

	if(c == '\r') {
		if(iflag & IGNCR) {
			spinlock_unlock_irqrestore(&tty->t_lock, irq);
			return;
		}

		if(iflag & ICRNL) c = '\n';
	} else if(c == '\n' && (iflag & INLCR)) {
		c = '\r';
	}

	if(iflag & IXON) {
		if(c == tty->t_termios.c_cc[VSTOP]) {
			tty->t_state |= TS_TTSTOP;
			spinlock_unlock_irqrestore(&tty->t_lock, irq);
			return;
		}

		if(c == tty->t_termios.c_cc[VSTART]) {
			tty->t_state &= ~TS_TTSTOP;
			start = 1;
			spinlock_unlock_irqrestore(&tty->t_lock, irq);

			if(start) tty_start(tty);
			return;
		}
	}

	if(canonical) {
		if(c == tty->t_termios.c_cc[VERASE]) {
			if(ttyq_unput(&tty->t_rawq) && (lflag & ECHO) && (lflag & ECHOE)) {
				tty_echo_locked(tty, '\b');
				tty_echo_locked(tty, ' ');
				tty_echo_locked(tty, '\b');
				start = 1;
			}

			spinlock_unlock_irqrestore(&tty->t_lock, irq);

			if(start) tty_start(tty);
			return;
		}

		if(c == tty->t_termios.c_cc[VKILL]) {
			ttyq_clear(&tty->t_rawq);

			if((lflag & ECHO) && (lflag & ECHOK)) {
				tty_echo_locked(tty, '\n');
				start = 1;
			}

			spinlock_unlock_irqrestore(&tty->t_lock, irq);

			if(start) tty_start(tty);
			return;
		}
	}

	if(!ttyq_put(&tty->t_rawq, c)) {
		spinlock_unlock_irqrestore(&tty->t_lock, irq);
		return;
	}

	delimiter = c == '\n';

	if(canonical && delimiter) {
		if(ttyq_space(&tty->t_canq) >= tty->t_rawq.count) {
			while(tty->t_rawq.count != 0) {
				unsigned char ch;

				ttyq_get(&tty->t_rawq, &ch);
				ttyq_put(&tty->t_canq, ch);
			}

			wakeup = 1;
		} else {
			ttyq_clear(&tty->t_rawq);
		}
	} else if(!canonical) {
		wakeup = 1;
	}

	if(lflag & ECHO) {
		if(tty_echo_locked(tty, c)) start = 1;
	}

	spinlock_unlock_irqrestore(&tty->t_lock, irq);

	if(wakeup) waiter_wakeup(&tty->t_rwait);

	if(start) tty_start(tty);
}

int tty_output_getc(struct tty* tty, unsigned char* data) {
	intr_flags_t irq;
	int	     result;

	irq = spinlock_lock_irqsave(&tty->t_lock);

	if((tty->t_state & TS_TTSTOP) || !ttyq_get(&tty->t_outq, data)) {
		result = 0;
	} else {
		result = 1;
	}

	spinlock_unlock_irqrestore(&tty->t_lock, irq);

	if(result) waiter_wakeup(&tty->t_wwait);

	return result;
}

void tty_flush(struct tty* tty, int flags) {
	intr_flags_t irq;

	irq = spinlock_lock_irqsave(&tty->t_lock);

	if(flags & TTY_FLUSH_INPUT) {
		ttyq_clear(&tty->t_rawq);
		ttyq_clear(&tty->t_canq);
	}

	if(flags & TTY_FLUSH_OUTPUT) ttyq_clear(&tty->t_outq);

	spinlock_unlock_irqrestore(&tty->t_lock, irq);

	if(flags & TTY_FLUSH_INPUT) waiter_wakeup(&tty->t_rwait);

	if(flags & TTY_FLUSH_OUTPUT) waiter_wakeup(&tty->t_wwait);
}

int tty_param(struct tty* tty, const struct termios* termios) {
	intr_flags_t		 irq;
	struct termios		 new_termios;
	const struct ttydev_ops* ops;
	int			 st;

	mutex_lock(&tty->t_param_mutex);

	new_termios = *termios;
	ops	    = tty->t_devops;

	st = 0;

	if(ops != NULL && ops->param != NULL) st = ops->param(tty, &new_termios);

	if(st == 0) {
		irq	       = spinlock_lock_irqsave(&tty->t_lock);
		tty->t_termios = new_termios;
		spinlock_unlock_irqrestore(&tty->t_lock, irq);
	}

	mutex_unlock(&tty->t_param_mutex);

	return st;
}
