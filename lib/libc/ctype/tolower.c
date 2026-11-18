#include <ctype.h>

int tolower(int c) {
	if(isupper(c)) return c + 0x20;

	return c;
}

int _tolower(int c) {
	return tolower(c);
}
