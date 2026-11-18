#include <ctype.h>

int isprint(int c) {
	if(isdigit(c)) return 1;
	if(isupper(c)) return 1;
	if(islower(c)) return 1;
	if(ispunct(c)) return 1;
	if(c == 0x20) return 1;

	return 0;
}
