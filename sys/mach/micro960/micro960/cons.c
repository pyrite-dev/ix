#include <sys/cons.h>
#include <sys/ins8250.h>

static volatile unsigned short* uart = (unsigned short*)0x220000;

static unsigned short baud_to_div(int baud) {
	return 10000000 / (16 * baud);
}

void cons_init(void) {
	unsigned short div = baud_to_div(9600);

	uart[IER] = 0;
	uart[LCR] = LC_DLAB;
	uart[DHR] = (div >> 8) & 0xff;
	uart[DLR] = (div >> 0) & 0xff;
	uart[LCR] = LC_PB_N | LC_SB | LC_DB_8;
	uart[IIR] = FC_ITL_14 | FC_CTFIFO | FC_CRFIFO | FC_EFIFO;
	uart[MCR] = MC_L | MC_O2 | MC_O1 | MC_RTS;

	while(!(uart[LSR] & LS_THRE));
	uart[DR] = 0xae;

	while(!(uart[LSR] & LS_DR));
	if(uart[DR] != 0xae) return; /* TODO: Serial is faulty */

	uart[MCR] = MC_O2 | MC_O1 | MC_RTS | MC_DTR;
}

void cons_putc(int c) {
	while(!(uart[LSR] & LS_THRE));
	uart[DR] = c;
}

int cons_getc(void) {
	while(!(uart[LSR] & LS_DR));
	return uart[DR];
}
