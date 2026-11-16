#ifndef __MACHINE_REGS_H__
#define __MACHINE_REGS_H__

struct iregs {
	unsigned int g0;
	unsigned int g1;
	unsigned int g2;
	unsigned int g3;
	unsigned int g4;
	unsigned int g5;
	unsigned int g6;
	unsigned int g7;
	unsigned int g8;
	unsigned int g9;
	unsigned int g10;
	unsigned int g11;
	unsigned int g12;
	unsigned int g13;
	unsigned int g14;
	unsigned int fp;

	unsigned int ac;
	unsigned int rip;
	unsigned int pc;
	unsigned int tc;
};

#endif
