volatile unsigned char* uart = (unsigned char*)0x220000;

void uart_init(void);
void uart_readable(void);
void uart_writable(void);
void uart_write(int c);

void uart_init(void) {
	uart[1 * 2] = 0x00;
	uart[3 * 2] = 0x80;
	uart[0 * 2] = 0x82;
	uart[1 * 2] = 0x00;
	uart[3 * 2] = 0x03;
	uart[2 * 2] = 0xc7;
	uart[4 * 2] = 0x0b;
	uart[4 * 2] = 0x1e;

	uart_writable();
	uart[0 * 2] = 0xae;

	uart_readable();
	if(uart[0 * 2] != 0xae) return;

	uart[4 * 2] = 0x0f;
}

void uart_readable(void){
	while(!(uart[5 * 2] & 1));
}

void uart_writable(void){
	while(!(uart[5 * 2] & 0x20));
}

void uart_write(int c){
	uart_writable();
	uart[0 * 2] = c;
}

void kmain(void){
	uart_init();
}
