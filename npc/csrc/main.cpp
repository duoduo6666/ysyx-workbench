#include <stdio.h>
#include <stdint.h>
#include <getopt.h>

#include "verilated.h"
#include "svdpi.h"

#include "Vysyx_2070017_CPU.h"

#define MEMORY_SIZE 65536

void init_expr();
void init_wp_pool();
void init_disassemble();
void disassemble(uint8_t *inst, size_t inst_len, uint64_t pc);
void sdb_loop();

extern Vysyx_2070017_CPU *top;
VerilatedContext *contextp;
bool exit_status = 1;
int program_status = 0;

extern uint8_t memory[MEMORY_SIZE];
uint8_t memory[MEMORY_SIZE] = {0};

void single_cycle() {
    top->clk = 0; top->eval(); contextp->timeInc(500);
    top->clk = 1; top->eval(); contextp->timeInc(500);
}

extern "C" void set_exit_status(int status) {
    if (exit_status) {
        exit_status = 0;
        program_status = status;
        if (program_status == 0) {
            printf("\033[1;32mHIT GOOD\033[0m\n");
        } else {
            printf("\033[1;32mBAD TRAP\033[0m\n");
        }
    }
}

char* img_file = NULL;
bool batch_mode = false;

void parse_args(int argc, char **argv) {
    struct option option_table[] = {
        {"batch"    , no_argument      , NULL, 'b'},
        // {"diff"     , required_argument, NULL, 'd'},
        {"help"     , no_argument      , NULL, 'h'},
        {0          , 0                , NULL,  0 },
    };
  int o;
  while ( (o = getopt_long(argc, argv, "-bh", option_table, 0)) != -1) {
    switch (o) {
      case 'b': batch_mode = true; break;
    //   case 'd': diff_so_file = optarg; break;
      case 1: img_file = optarg; return;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        // printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\n");
        exit(0);
    }
  }
}

void init(int argc, char **argv) {
    parse_args(argc, argv);

    if (img_file != NULL) {
        FILE* fp = fopen(argv[1], "r");
        if (fp == NULL) {
            printf("Can't read %s file\n", argv[1]);
        }
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

    contextp = new VerilatedContext;
    contextp->traceEverOn(true);
    contextp->commandArgs(argc, argv);
    top = new Vysyx_2070017_CPU{contextp};

    top->rst = 1;
    single_cycle();
    top->rst = 0;
}

void cpu_exec(int n, bool enable_disassemble) {
    uint32_t pc;
    while (exit_status && n != 0) {
        pc = top->pc - 0x80000000;
        if (pc > MEMORY_SIZE) {
            if (top->pc == 0) {
                printf("未实现的指令 0x%x\n", top->inst);
            }
            printf("pc error\n");
            break;
        }
        top->inst = (*(uint32_t*)(memory+pc));
        if (enable_disassemble) {
            disassemble((uint8_t*)&(top->inst), 4, top->pc);
        }
        single_cycle();
        n--;
    }
}

int main(int argc, char **argv) {
    init(argc, argv);

    if (batch_mode) {
        cpu_exec(-1, false);
    } else {
        init_expr();
        init_wp_pool();
        init_disassemble();
        sdb_loop();
    }
    top->final();
    delete top;
    delete contextp;

    return program_status;
}
