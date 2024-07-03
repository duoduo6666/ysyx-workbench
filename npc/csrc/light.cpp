#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <nvboard.h>

#include "verilated.h"

#include "Vlight.h"

static Vlight* top;
static VerilatedContext* contextp;

void nvboard_bind_all_pins(Vlight* top);

void single_cycle() {
  top->clk = 0; top->eval(); contextp->timeInc(500);
  top->clk = 1; top->eval(); contextp->timeInc(500);
}

void reset(int n) {
  top->rst = 1;
  while (n -- > 0) single_cycle();
  top->rst = 0;
}

int main(int argc, char** argv) {
	contextp = new VerilatedContext;
	contextp->traceEverOn(true);
	contextp->commandArgs(argc, argv);
	top = new Vlight{contextp};
	
	nvboard_bind_all_pins(top);
	nvboard_init();

	unsigned int i = 60 * 10000000;
	reset(10);
	while (i) {
		// contextp->timeInc(1);
		single_cycle();
		nvboard_update();
		i--;
	}


	nvboard_quit();
	top->final();

	delete top;
	delete contextp;
	return 0;
}
