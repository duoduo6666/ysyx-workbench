#include <stdio.h>
#include <stdint.h>
#include <getopt.h>

#include "verilated.h"
#include "svdpi.h"

#include "Vysyx_2070017_CPU.h"

#define MEMORY_SIZE 65536

typedef uint32_t word_t;
void init_expr();
void init_wp_pool();
void init_disassemble();
void disassemble(uint8_t *inst, size_t inst_len, uint64_t pc);
void init_ftrace();
void ftrace_check(word_t pc, word_t next_pc);
void init_difftest();
void difftest_check_reg();
void sdb_loop();

extern Vysyx_2070017_CPU *top;
VerilatedContext *contextp;
extern bool exit_status;
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

extern char* img_file;
extern char* elf_file;
extern char* diff_so_file;
char* img_file = NULL;
char* elf_file = NULL;
char* diff_so_file = NULL;
bool batch_mode = false;

void parse_args(int argc, char **argv) {
    struct option option_table[] = {
        {"batch"    , no_argument      , NULL, 'b'},
        {"elf"      , required_argument, NULL, 'e'},
        {"diff"     , required_argument, NULL, 'd'},
        {"help"     , no_argument      , NULL, 'h'},
        {0          , 0                , NULL,  0 },
    };
  int o;
  while ( (o = getopt_long(argc, argv, "-bh", option_table, 0)) != -1) {
    switch (o) {
      case 'b': batch_mode = true; break;
      case 'e': elf_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 1: img_file = optarg; return;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-e,--elf                elf file\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
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

// iringbuf
static word_t iringbuf[10] = {0};
static int iringbuf_end = 0;
void display_iringbuf_inst(word_t pc, bool is_end) {
    if (is_end) {
        printf("-->");
    } else {
        printf("   ");
    }
    disassemble(memory+pc, 4, pc);
}
void display_iringbuf() {
    printf("iringbuf: \n");
    if (iringbuf[(iringbuf_end+1) % (sizeof(iringbuf)/sizeof(word_t))] == 0){
        for (int i = 0; i < iringbuf_end-1; i++) {
        display_iringbuf_inst(iringbuf[i], false);
        }
        display_iringbuf_inst(iringbuf[iringbuf_end-1], true);
    } else {
        for (int i = iringbuf_end; i < (sizeof(iringbuf)/sizeof(word_t)); i++) {
        display_iringbuf_inst(iringbuf[i], false);
        }
        for (int i = 0; i < iringbuf_end-1; i++) {
        display_iringbuf_inst(iringbuf[i], false);
        }
        display_iringbuf_inst(iringbuf[iringbuf_end-1], true);
    }
}

void cpu_exec(int n, bool enable_disassemble) {
    uint32_t memory_offset;
    while (exit_status && n != 0) {
        memory_offset = top->pc - 0x80000000;
        if (memory_offset > MEMORY_SIZE) {
            if (top->pc == 0) {
                printf("可能是未实现的指令 0x%08x\n", top->inst);
            }
            printf("pc: 0x%08x 无法访问pc指向的内存\n", top->pc);
            display_iringbuf();
            break;
        }
        top->inst = (*(uint32_t*)(memory+memory_offset));

        // itrace
        if (enable_disassemble) {
            disassemble((uint8_t*)&(top->inst), 4, top->pc);
        }
        iringbuf[iringbuf_end] = memory_offset;
        iringbuf_end = (iringbuf_end+1) % (sizeof(iringbuf)/sizeof(word_t));
        word_t old_pc = top->pc;
        single_cycle();
        word_t new_pc = top->pc;
        if (exit_status) {
            difftest_check_reg();
            ftrace_check(old_pc, new_pc);
        }
        n--;
    }
}

int main(int argc, char **argv) {
    init(argc, argv);
    init_difftest();

    if (batch_mode) {
        cpu_exec(-1, false);
    } else {
        init_expr();
        init_wp_pool();
        init_disassemble();
        init_ftrace();
        sdb_loop();
    }
    top->final();
    delete top;
    delete contextp;

    return program_status;
}
