#include <ctype.h>

int toupper(int c) {
	if(islower(c)) return c - 0x20;

	return c;
}

int _toupper(int c) {
	return toupper(c);
}
