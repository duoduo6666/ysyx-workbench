#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "verilated.h"

#include "VDoubleControlSwitch.h"

int main(int argc, char** argv) {
	VerilatedContext* contextp = new VerilatedContext;
	contextp->traceEverOn(true);
	contextp->commandArgs(argc, argv);
	VDoubleControlSwitch* top = new VDoubleControlSwitch{contextp};
		
	int i = 100;
	while (i) {
		contextp->timeInc(1);
		int a = rand() & 1;
		int b = rand() & 1;
		top->a = a;
		top->b = b;
		top->eval();
		printf("a = %d, b = %d, f = %d\n", a, b, top->f);
		assert(top->f == (a ^ b));
		i--;
	}

	top->final();

		
	delete top;
	delete contextp;
	return 0;
}
