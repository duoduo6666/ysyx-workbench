#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Disassembler.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Core.h>

LLVMDisasmContextRef DCR;

void init_disassemble() {
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllDisassemblers();

    DCR = LLVMCreateDisasm("riscv32-unknown-unknown-elf", NULL, 0, NULL, NULL);
    if (!DCR) {
        fprintf(stderr, "Failed to create disassembler\n");
        return;
    }
}
void disassemble(uint8_t *inst, size_t inst_len, uint64_t pc) {
    char assemble[256] = {0};

    size_t ret = LLVMDisasmInstruction(DCR, inst, inst_len, pc, assemble, sizeof(assemble));

    if (ret > 0) {
        printf("0x%x: %08x %s\n", (uint32_t)pc, *(uint32_t*)inst, assemble);
    } else {
        printf("Failed to disassemble instruction\n");
    }
}
