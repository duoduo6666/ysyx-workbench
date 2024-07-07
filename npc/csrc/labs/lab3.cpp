#include <nvboard.h>

#include "verilated.h"

#include "Vlab3.h"

void nvboard_bind_all_pins(Vlab3* top);

int main(int argc, char** argv) {
	VerilatedContext* contextp = new VerilatedContext;
	contextp->traceEverOn(true);
	contextp->commandArgs(argc, argv);
	Vlab3* top = new Vlab3{contextp};
	
	nvboard_bind_all_pins(top);
	nvboard_init();

	while (1) {
		top->eval();
		nvboard_update();
		contextp->timeInc(1);
	}


	nvboard_quit();
	top->final();

	delete top;
	delete contextp;
	return 0;
}
