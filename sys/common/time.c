#include <sys/time.h>

int		       hz    = 0;
volatile unsigned long ticks = 0;

void microtime(struct timeval* tv) {
	unsigned long t = ticks;

	if(hz == 0) {
		tv->tv_sec  = 0;
		tv->tv_usec = 0;
		return;
	}

	tv->tv_sec  = (time_t)(t / (unsigned long)hz);
	tv->tv_usec = (long)(((unsigned long long)(t % (unsigned long)hz) * 1000000) / (unsigned long)hz);
}

unsigned long ticktime(unsigned int ms) {
	if(hz == 0) return 0;

	return (unsigned long)(((unsigned long long)hz * ms + 999) / 1000);
}
