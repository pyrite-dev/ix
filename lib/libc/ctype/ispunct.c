#include <ctype.h>

int ispunct(int c) {
	if(0x21 <= c && c <= 0x2f) return 1;
	if(0x3a <= c && c <= 0x40) return 1;
	if(0x5b <= c && c <= 0x60) return 1;
	if(0x7b <= c && c <= 0x7e) return 1;

	return 0;
}
