#include <sys/cons.h>

#include "../../../dev/ns.h"

static volatile unsigned short* uart = (unsigned short*)0x220000;

static unsigned short baud_to_div(int baud) {
	return 10000000 / (16 * baud);
}

void cons_init(void) {
	unsigned short div = baud_to_div(9600);

	uart[NS_IER] = 0;
	uart[NS_LCR] = NS_LC_DLAB;
	uart[NS_DHR] = (div >> 8) & 0xff;
	uart[NS_DLR] = (div >> 0) & 0xff;
	uart[NS_LCR] = NS_LC_PB_N | NS_LC_SB | NS_LC_DB_8;
	uart[NS_IIR] = NS_FC_ITL_14 | NS_FC_CTFIFO | NS_FC_CRFIFO | NS_FC_EFIFO;
	uart[NS_MCR] = NS_MC_L | NS_MC_O2 | NS_MC_O1 | NS_MC_RTS;

	while(!(uart[NS_LSR] & NS_LS_THRE));
	uart[NS_DR] = 0xae;

	while(!(uart[NS_LSR] & NS_LS_DR));
	if(uart[NS_DR] != 0xae) return; /* TODO: Serial is faulty */

	uart[NS_MCR] = NS_MC_O2 | NS_MC_O1 | NS_MC_RTS | NS_MC_DTR;
}

void cons_putc(int c) {
	while(!(uart[NS_LSR] & NS_LS_THRE));
	uart[NS_DR] = c;
}

int cons_getc(void) {
	while(!(uart[NS_LSR] & NS_LS_DR));
	return uart[NS_DR];
}
