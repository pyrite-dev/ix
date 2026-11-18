#include <ctype.h>

int isspace(int c) {
	if(c == 0x20) return 1;
	if(c == 0x0c) return 1;
	if(c == 0x0a) return 1;
	if(c == 0x0d) return 1;
	if(c == 0x09) return 1;
	if(c == 0x0b) return 1;

	return 0;
}
