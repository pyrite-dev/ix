#include <ctype.h>

int isgraph(int c) {
	if(isdigit(c)) return 1;
	if(isupper(c)) return 1;
	if(islower(c)) return 1;
	if(ispunct(c)) return 1;

	return 0;
}
