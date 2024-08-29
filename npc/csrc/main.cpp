#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <time.h>

#include "verilated.h"
#include "svdpi.h"

#include "Vysyx_2070017_CPU.h"

#define MEMORY_SIZE 0x8000000

// #define CONFIG_ITARCE
// #define CONFIG_MTARCE
// #define CONFIG_FTARCE
// #define CONFIG_DIFFTEST

#define DEVICE_SERIAL_ADDR 0xa00003F8
#define DEVICE_RTC_ADDR  0xa0000048

typedef uint32_t word_t;
void init_expr();
void init_wp_pool();
void init_disassemble();
void disassemble(uint8_t *inst, size_t inst_len, uint64_t pc);
void init_ftrace();
void ftrace_check(word_t pc, word_t next_pc);
void init_difftest();
void difftest_set_skip();
void difftest_check_reg();
void display_reg();
void display_iringbuf();
void sdb_loop();

extern Vysyx_2070017_CPU *top;
VerilatedContext *contextp;
extern bool exit_status;
bool exit_status = 1; // 1 = run, 0 == stop
int program_status = 0;

extern uint8_t memory[MEMORY_SIZE];
uint8_t memory[MEMORY_SIZE] = {0};

bool new_clk = 0;
void single_cycle() {
    top->clk = 0; top->eval(); contextp->timeInc(500);
    top->clk = 1; top->eval(); contextp->timeInc(500);
    new_clk = 1;
}

extern "C" void set_exit_status(int status) {
    if (exit_status) {
        exit_status = 0;
        program_status = status;
        if (program_status == 0) {
            printf("\033[1;32mHIT GOOD\033[0m\n");
        } else {
            display_reg();
#ifdef CONFIG_ITARCE
            display_iringbuf();
#endif
            printf("\033[1;31mBAD TRAP\033[0m\n");
        }
    }
}
extern "C" int pmem_read(int raddr) {
    static int old_data = 0;
    if (new_clk == false) return old_data;
    new_clk = false;
    
    struct timespec time;
    if (raddr == DEVICE_RTC_ADDR || raddr == DEVICE_RTC_ADDR+4) {
        assert(timespec_get(&time, TIME_UTC) == TIME_UTC);
        old_data = time.tv_nsec;
#ifdef CONFIG_DIFFTEST
        difftest_set_skip();
#endif
        uint64_t us = time.tv_sec * 1000000 + time.tv_nsec / 1000;
        if (raddr == DEVICE_RTC_ADDR) {
            // printf("get rtc\n");
            old_data = (uint32_t)us;
            return (uint32_t)us;
        }
        else if (raddr == DEVICE_RTC_ADDR+4) {
            old_data = (uint32_t)(us >> 32);
            return (uint32_t)(us >> 32);
        }
    }

    word_t memory_offset = raddr - 0x80000000;
    if (memory_offset >= MEMORY_SIZE) {
        printf("error address 0x%08x\n", raddr);
        set_exit_status(-1);
    }
#ifdef CONFIG_MTARCE
    printf("memory read 0x%08x: 0x%08x\n", raddr, *(uint32_t*)(&(memory[memory_offset])));
#endif
    old_data = *(uint32_t*)(&(memory[memory_offset]));
    return *(uint32_t*)(&(memory[memory_offset]));
}
extern "C" void pmem_write(int waddr, int wdata, char wmask) {
    if (new_clk == false) return;
    new_clk = false;

    if (waddr == DEVICE_SERIAL_ADDR) {
        if (wmask != 1) assert(0);
            // printf("write SERIAL\n");
        putchar(wdata & 0xff);
        fflush(stdout);
#ifdef CONFIG_DIFFTEST
        difftest_set_skip();
#endif
        return;
    }
    static word_t pc;
    pc = top->pc;
    word_t memory_offset = waddr - 0x80000000;
    if (memory_offset >= MEMORY_SIZE) {
        printf("error address 0x%08x\n", waddr);
        set_exit_status(-1);
        return;
    }
    switch (wmask) {
    case 0b0001:
#ifdef CONFIG_MTARCE
        printf("memory write 0x%08x: 0x%08x\n", waddr, (uint8_t)wdata);
#endif
        memory[memory_offset] = (uint8_t)wdata;
        break;
    case 0b0011:
#ifdef CONFIG_MTARCE
        printf("memory write 0x%08x: 0x%08x\n", waddr, (uint16_t)wdata);
#endif
        *(uint16_t*)(&(memory[memory_offset])) = (uint16_t)wdata;
        break;
    case 0b1111:
#ifdef CONFIG_MTARCE
        printf("memory write 0x%08x: 0x%08x\n", waddr, wdata);
#endif
        *(uint32_t*)(&(memory[memory_offset])) = wdata;
        break;
    default:
        assert(0);
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
    };
    int o;
    while ( (o = getopt_long(argc, argv, "be:d:h", option_table, 0)) != -1) {
        switch (o) {
        case 'b': batch_mode = true; break;
        case 'e': elf_file = optarg; break;
        case 'd': diff_so_file = optarg; break;
        default:
            printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
            printf("\t-b,--batch              run with batch mode\n");
            printf("\t-e,--elf                elf file\n");
            printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
            printf("\n");
            exit(0);
        }
    }
    for (int index = optind; index < argc; index++) {
        img_file = argv[index];
    }
}

void init(int argc, char **argv) {
    parse_args(argc, argv);

    if (img_file != NULL) {
        FILE* fp = fopen(img_file, "r");
        if (fp == NULL) {
            printf("Can't read %s file\n", img_file);
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


#ifdef CONFIG_ITARCE
static word_t iringbuf[10] = {0};
static int iringbuf_end = 0;
void display_iringbuf_inst(word_t pc, bool is_end) {
    if (is_end) {
        printf("-->");
    } else {
        printf("   ");
    }
    disassemble(memory+(pc-0x80000000), 4, pc);
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
#endif

void cpu_exec(int n, bool enable_disassemble) {
    uint32_t memory_offset;
    while (exit_status && n != 0) {
        memory_offset = top->pc - 0x80000000;
        if (memory_offset > MEMORY_SIZE) {
            if (top->pc == 0) {
                printf("可能是未实现的指令 0x%08x\n", top->inst);
            }
            printf("pc: 0x%08x 无法访问pc指向的内存\n", top->pc);
#ifdef CONFIG_ITARCE
            display_iringbuf();
#endif
            break;
        }
        top->inst = (*(uint32_t*)(memory+memory_offset));

#ifdef CONFIG_ITARCE
        if (enable_disassemble) {
            disassemble((uint8_t*)&(top->inst), 4, top->pc);
        }
        iringbuf[iringbuf_end] = top->pc;
        iringbuf_end = (iringbuf_end+1) % (sizeof(iringbuf)/sizeof(word_t));
#endif

        word_t old_pc = top->pc;
        single_cycle();
        word_t new_pc = top->pc;
#ifdef CONFIG_DIFFTEST
        if (exit_status) difftest_check_reg();
#endif
#ifdef CONFIG_FTARCE
        if (exit_status) ftrace_check(old_pc, new_pc);
#endif
        if (n > 0) n--;
    }
}

int main(int argc, char **argv) {
    init(argc, argv);
#ifdef CONFIG_FTARCE
    init_ftrace();
#endif
#ifdef CONFIG_ITARCE
    init_disassemble();
#endif
#ifdef CONFIG_DIFFTEST
    init_difftest();
#endif

    if (batch_mode) {
        cpu_exec(-1, false);
    } else {
        init_expr();
        init_wp_pool();
        sdb_loop();
    }
    top->final();
    delete top;
    delete contextp;

    return program_status;
}
