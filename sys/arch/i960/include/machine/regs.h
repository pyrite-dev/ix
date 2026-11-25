#ifndef __MACHINE_REGS_H__
#define __MACHINE_REGS_H__

typedef unsigned int reg_t; /* biggest register */

struct iregs {
	reg_t g0;
	reg_t g1;
	reg_t g2;
	reg_t g3;
	reg_t g4;
	reg_t g5;
	reg_t g6;
	reg_t g7;
	reg_t g8;
	reg_t g9;
	reg_t g10;
	reg_t g11;
	reg_t g12;
	reg_t g13;
	reg_t g14;
	reg_t pfp;

	reg_t ac;
	reg_t rip;
	reg_t pc;
	reg_t tc;
};

#endif
