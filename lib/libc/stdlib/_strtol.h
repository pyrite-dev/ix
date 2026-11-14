#include <stdlib.h>
#include <string.h>

RETTYPE FUNCNAME(const char* nptr, char** endptr, int base) {
	RETTYPE r = 0;
	int	i;
	RETTYPE s      = 1;
	char	num[]  = "0123456789";
	char	alp[]  = "abcdefghijklmnopqrstuvwxyz";
	char	alpu[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	if(endptr != NULL) *endptr = (char*)nptr + strlen(nptr);

	if(base < 2 || base > 36) return 0;

	for(i = 0; i < strlen(nptr); i++) {
		int n = -1;

		if('0' <= nptr[i] && nptr[i] <= num[(base > 10 ? 10 : base) - 1]) {
			n = nptr[i] - '0';
		} else if(base > 10 && 'a' <= nptr[i] && nptr[i] <= alp[base - 11]) {
			n = nptr[i] - 'a' + 10;
		} else if(base > 10 && 'A' <= nptr[i] && nptr[i] <= alpu[base - 11]) {
			n = nptr[i] - 'A' + 10;
#ifndef UNSIGNED
		} else if(i == 0 && nptr[i] == '-') {
			s = -1;
			continue;
#endif
		} else if(i == 0 && nptr[i] == '+') {
		} else {
			if(endptr != NULL) *endptr = (char*)&nptr[i];
			break;
		}

		r = r * base;
		r = r + n;
	}

	return r * s;
}
