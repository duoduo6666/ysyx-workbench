#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include "Vysyx_2070017_CPU.h"
#include "Vysyx_2070017_CPU___024root.h"

#define MEMORY_SIZE 65536

extern Vysyx_2070017_CPU *top;
extern uint8_t memory[MEMORY_SIZE];
extern bool exit_status;
extern const char *reg_names[];
extern char* diff_so_file;
char default_diff_so_file[] = "/build/riscv32-nemu-interpreter-so";
void disassemble(uint8_t *inst, size_t inst_len, uint64_t pc);
extern "C" void set_exit_status(int status);

typedef uint32_t word_t;
typedef uint32_t paddr_t;
void (*difftest_memcpy)(paddr_t, void*, size_t, bool);
void (*difftest_regcpy)(void*, bool);
void (*difftest_pccpy)(void*, bool);
void (*difftest_exec)(uint64_t);

void init_difftest() {
    if (diff_so_file == NULL) {
        diff_so_file = (char*)calloc(strlen(getenv("NEMU_HOME")) + sizeof(default_diff_so_file), sizeof(char));
        strcpy(diff_so_file, getenv("NEMU_HOME"));
        strcat(diff_so_file, default_diff_so_file);
    }
    void *handle = dlopen(diff_so_file, RTLD_LAZY);
    assert(handle != NULL);
    difftest_memcpy = (void (*)(paddr_t, void*, size_t, bool))dlsym(handle, "difftest_memcpy");
    assert(difftest_memcpy != NULL);
    difftest_regcpy = (void (*)(void*, bool))dlsym(handle, "difftest_regcpy");
    assert(difftest_regcpy != NULL);
    difftest_pccpy = (void (*)(void*, bool))dlsym(handle, "difftest_pccpy");
    assert(difftest_pccpy != NULL);
    difftest_exec = (void (*)(uint64_t))dlsym(handle, "difftest_exec");
    assert(difftest_exec != NULL);

    difftest_memcpy(0x80000000, memory, MEMORY_SIZE, 1);
    difftest_regcpy(top->rootp->ysyx_2070017_CPU__DOT__rf_data, 1);
    difftest_pccpy(&(top->pc), 1);
}

void difftest_check_reg() {
    word_t ref_pc;
    word_t ref_regs[32];
    difftest_exec(1);
    difftest_pccpy(&ref_pc, 0);
    difftest_regcpy(ref_regs, 0);
    if (top->pc != ref_pc) {
        printf("difftest pc reg error: \nref: 0x%x \ndut: 0x%x\n", ref_pc, top->pc);
        set_exit_status(-1);
    }
    for (size_t i = 0; i < 16; i++) {
        if (top->rootp->ysyx_2070017_CPU__DOT__rf_data[i] != ref_regs[i]) {
            printf("difftest %s reg error: \nref: 0x%x \ndut: 0x%x\n", reg_names[i], ref_regs[i], top->rootp->ysyx_2070017_CPU__DOT__rf_data[i]);
            set_exit_status(-1);
        }
    }
}