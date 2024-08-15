#include <stdio.h>
#include <stdint.h>

#include "verilated.h"
#include "svdpi.h"

#include "Vysyx_2070017_CPU.h"

#define MEMORY_SIZE 65536

Vysyx_2070017_CPU *top;
VerilatedContext *contextp;
bool exit_status = 1;
int program_status = 0;

uint8_t memory[MEMORY_SIZE] = {0};

void single_cycle() {
    top->clk = 0; top->eval(); contextp->timeInc(500);
    top->clk = 1; top->eval(); contextp->timeInc(500);
}

extern "C" void set_exit_status() {
    exit_status = 0;
    // program_status = status;
}

int main(int argc, char **argv) {
    contextp = new VerilatedContext;
    contextp->traceEverOn(true);
    contextp->commandArgs(argc, argv);
    top = new Vysyx_2070017_CPU{contextp};

    FILE* fp = fopen(argv[1], "r");
    if (fp != NULL) {
        assert(fp != NULL);
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        assert(file_size < MEMORY_SIZE);
        assert(fread(memory, sizeof(uint8_t), file_size, fp) == file_size);
        fclose(fp);
    } else {
        ((uint32_t*)memory)[0] = 0x00100073;
    }

    top->rst = 1;
    single_cycle();
    top->rst = 0;
    uint32_t pc;

    while (exit_status) {
        pc = top->pc - 0x80000000;
        top->inst = ((uint32_t*)memory)[pc/4];
        single_cycle();
    }

    top->final();
    delete top;
    delete contextp;
    return program_status;
}
