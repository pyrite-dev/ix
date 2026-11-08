#include <sys/cons.h>
#include <sys/ins8250.h>

static volatile unsigned char* uart = (unsigned char*)0x220000;

static unsigned short baud_to_div(int baud){
	return 10000000/(16 * baud);
}

void consinit(void){
	unsigned short div = baud_to_div(9600);

	uart[IER * 2] = 0;
	uart[LCR * 2] = LC_DLAB;
	uart[DHR * 2] = (div >> 8) & 0xff;
	uart[DLR * 2] = (div >> 0) & 0xff;
	uart[LCR * 2] = LC_PB_N | LC_SB | LC_DB_8;
	uart[IIR * 2] = FC_ITL_14 | FC_CTFIFO | FC_CRFIFO | FC_EFIFO;
	uart[MCR * 2] = MC_L | MC_O2 | MC_O1 | MC_RTS;

	while(!(uart[LSR * 2] & LS_THRE));
	uart[DR * 2] = 0xae;

	while(!(uart[LSR * 2] & LS_DR));
	if(uart[DR * 2] != 0xae) return; /* TODO: Serial is faulty */

	uart[MCR * 2] = MC_O2 | MC_O1 | MC_RTS | MC_DTR;
}

void consputc(int c){
	while(!(uart[LSR * 2] & LS_THRE));
	uart[DR * 2] = c;
}

int consgetc(void){
	while(!(uart[LSR * 2] & LS_DR));
	return uart[DR * 2];
}
