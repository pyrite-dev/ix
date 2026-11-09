#include <string.h>

char* strchr(const char* s1, int c) {
	while(*s1) {
		if(*s1 == c) return (char*)s1;
		s1++;
	}

	return NULL;
}
