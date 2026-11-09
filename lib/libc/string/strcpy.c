#include <string.h>

char* strcpy(char* s1, const char* s2) {
	memcpy(s1, s2, strlen(s2) + 1);

	return s1;
}
