volatile unsigned char* uart = (unsigned char*)0x220000;
volatile unsigned char* timer = (unsigned char*)0x200000;

void uart_readable(void);
void uart_writable(void);
void uart_write(int c);

static void uart_init(void) {
	uart[1 * 2] = 0x00;
	uart[3 * 2] = 0x80;
	uart[0 * 2] = 0x41;
	uart[1 * 2] = 0x00;
	uart[3 * 2] = 0x03;
	uart[2 * 2] = 0xc7;
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

static void timer_init(void){
	timer[3 * 2] = (3 << 4) | (2 << 1);
	timer[0 * 2] = 0xe8;
	timer[0 * 2] = 0x03;
}

void kmain(void){	
	uart_init();
	timer_init();
}
