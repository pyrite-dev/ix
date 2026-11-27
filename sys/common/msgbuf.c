#include <sys/msgbuf.h>

#define MSGBUF_SIZE 16384

static char	    msgbuf[MSGBUF_SIZE];
static unsigned int msgbuf_written;
static unsigned int msgbuf_read;

void msgbuf_putc(int c) {
	msgbuf[msgbuf_written % MSGBUF_SIZE] = (char)c;
	msgbuf_written++;
}

int msgbuf_peek(unsigned int* pos) {
	int c;

	if(msgbuf_written - *pos > MSGBUF_SIZE) *pos = msgbuf_written - MSGBUF_SIZE;
	if(*pos == msgbuf_written) return -1;

	c = (unsigned char)msgbuf[*pos % MSGBUF_SIZE];
	(*pos)++;
	return c;
}

int msgbuf_getc(void) {
	return msgbuf_peek(&msgbuf_read);
}
