#include <stdio.h>

#include "verilated.h"
#include "svdpi.h"

#include "Vysyx_2070017_CPU.h"

Vysyx_2070017_CPU *top;
VerilatedContext *contextp;
bool status;

uint32_t mem[256] = {
    0x00108093, 0x00108093, 0x00108093, 0x00108093,
    0x00100073,
};

void single_cycle() {
    top->clk = 0; top->eval(); contextp->timeInc(500);
    top->clk = 1; top->eval(); contextp->timeInc(500);
}

extern "C" void set_stop_status() {
    status = 0;
}

int main(int argc, char **argv) {
    contextp = new VerilatedContext;
    contextp->traceEverOn(true);
    contextp->commandArgs(argc, argv);
    top = new Vysyx_2070017_CPU{contextp};
    top->rst = 1;
    single_cycle();
    top->rst = 0;
    uint32_t pc;

    status = 1;
    while (status) {
        pc = top->pc - 0x80000000;
        top->inst = mem[pc/4];
        single_cycle();
    }

    top->final();
    delete top;
    delete contextp;
    return 0;
}
