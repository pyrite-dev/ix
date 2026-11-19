#ifndef __SYS_TERMIOS_H__
#define __SYS_TERMIOS_H__

struct termios;

typedef unsigned int  tcflag_t;
typedef unsigned int  speed_t;
typedef unsigned char cc_t;

#define NCCS 16

#define IGNBRK (1 << 0)
#define BRKINT (1 << 1)
#define IGNPAR (1 << 2)
#define PARMRK (1 << 3)
#define INPCK (1 << 4)
#define ISTRIP (1 << 5)
#define INLCR (1 << 6)
#define IGNCR (1 << 7)
#define ICRNL (1 << 8)
#define IXON (1 << 9)
#define IXOFF (1 << 10)
#define IXANY (1 << 11)

#define OPOST (1 << 0)
#define ONLCR (1 << 1)
#define OCRNL (1 << 2)
#define ONOCR (1 << 3)
#define ONLRET (1 << 4)
#define TAB3 (1 << 5)

#define CSIZE (3 << 0)
#define CS5 (0 << 0)
#define CS6 (1 << 0)
#define CS7 (2 << 0)
#define CS8 (3 << 0)

#define CSTOPB (1 << 3)
#define CREAD (1 << 4)
#define PARENB (1 << 5)
#define PARODD (1 << 6)
#define HUPCL (1 << 7)
#define CLOCAL (1 << 8)

#define ISIG (1 << 0)
#define ICANON (1 << 1)
#define ECHO (1 << 2)
#define ECHOE (1 << 3)
#define ECHOK (1 << 4)
#define ECHONL (1 << 5)
#define NOFLSH (1 << 6)
#define IEXTEN (1 << 7)

#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSTART 7
#define VSTOP 8
#define VEOL 9
#define VSUSP 10
#define VWERASE 11
#define VLNEXT 12
#define VEOL2 13

#define B0 0
#define B300 300
#define B600 600
#define B1200 1200
#define B2400 2400
#define B4800 4800
#define B9600 9600
#define B19200 19200
#define B38400 38400
#define B57600 57600
#define B115200 115200

struct termios {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t	 c_cc[NCCS];

	speed_t c_ispeed;
	speed_t c_ospeed;
};

#endif
