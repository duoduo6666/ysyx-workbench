#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <nvboard.h>
#include "verilated.h"

#include "VDoubleControlSwitch.h"

void nvboard_bind_all_pins(VDoubleControlSwitch* top);

int main(int argc, char** argv) {
	VerilatedContext* contextp = new VerilatedContext;
	contextp->traceEverOn(true);
	contextp->commandArgs(argc, argv);
	VDoubleControlSwitch* top = new VDoubleControlSwitch{contextp};
		
	
  	nvboard_bind_all_pins(top);
  	nvboard_init();
	while (1) {
		contextp->timeInc(1);
		nvboard_update();
		top->eval();
	}

	top->final();

		
	delete top;
	delete contextp;
	return 0;
}
