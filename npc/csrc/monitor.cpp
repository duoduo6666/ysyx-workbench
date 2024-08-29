#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "Vysyx_2070017_CPU.h"
#include "Vysyx_2070017_CPU___024root.h"
#define MEMORY_SIZE 0x8000000
#define MEMORY_LEFT  0x80000000
#define MEMORY_RIGHT MEMORY_LEFT + MEMORY_SIZE
typedef uint32_t word_t;
Vysyx_2070017_CPU *top;
extern uint8_t memory[MEMORY_SIZE];
void cpu_exec(int n, bool enable_disassemble);
word_t expr(char *e, bool *success);
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  char expr[32*32];
  word_t result;
} WP;
void free_wp(int no);
WP* new_wp(char* e);
void display_wp();

extern const char *reg_names[];
const char *reg_names[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void display_reg() {
  printf("pc: 0x%x\n", top->pc);
  for (int i=0; i<16; i++) {
    printf("%s: 0x%x\n", reg_names[i], top->rootp->ysyx_2070017_CPU__DOT__rf_data[i]);
  }
}

word_t get_reg(char* reg, bool *success) {
    if (strcmp(reg, "pc") == 0) {
        *success = true;
        return top->pc;
    }
    for (int i = 0; i < 32; i++){
        if (strcmp(reg_names[i], reg) == 0) {
            *success = true;
            return top->rootp->ysyx_2070017_CPU__DOT__rf_data[i];
        }
    }
    *success = false;
    return 0;
}

static int cmd_c(char *args) {
    cpu_exec(-1, false);
    return 0;
}


static int cmd_q(char *args) {
    return -1;
}

static int cmd_help(char *args);

int cmd_si(char *args) {
    char *endptr;
    long num = 1;
    if (args) {num = strtol(args, &endptr, 10);}
    cpu_exec(num, true);
    return 0;
}

int cmd_info(char *args) {
    if (!args) {printf("r: 打印寄存器状态, w: 打印监视点信息"); return 1;}
    if (args[0] == 'r') {display_reg();}
    if (args[0] == 'w') {display_wp();}
    return 0;
}

int cmd_x(char *args) {
    if (!args) {printf("x N EXPR 求出表达式EXPR的值, 将结果作为起始内存地址, 以十六进制形式输出连续的N个4字节"); return 1;}
    int num = 0;
    int ptr = 0;
    sscanf(args, "%d %x", &num, &ptr);
    if ((MEMORY_LEFT > ptr) | (ptr+num*4-1 > MEMORY_RIGHT)) {printf("内存地址应在 0x%x-0x%x 之间\n", MEMORY_LEFT, MEMORY_RIGHT); return 1;}
    ptr -= MEMORY_LEFT;
    for (int i = 0; i < num; i++){
        printf("0x%x: 0x%x\n", ptr+i*4, ((word_t*)memory)[ptr+i]);
    }
    return 0;
}

int cmd_p(char *args) {
    if (!args) {printf("p EXPR 求出表达式EXPR的值"); return 1;}
    bool success;
    word_t result = expr(args, &success);
    if (success) {printf("%u\n", result);}
    else {printf("计算失败\n");}
    return 0;
}

int cmd_w(char *args) {
    if (!args) {printf("w EXPR 当表达式EXPR的值发生变化时, 暂停程序执行"); return 1;}
    WP* wp = new_wp(args);
    printf("watch point NO.%d\n", wp->NO);
    return 0;
}

int cmd_d(char *args) {
    if (!args) {printf("d N 删除序号为N的监视点"); return 1;}
    int n = atoi(args);
    free_wp(n);
    return 0;
}

int cmd_test_expr(char *args) {
    if (!args) {printf("test_expr filename"); return 1;}

    bool success;
    static char buf[65536] = {};
    word_t result;

    FILE *fp = fopen(args, "r");

    while (fscanf(fp, "%u %[^\n]", &result, buf) == 2) {
        if (expr(buf, &success) != result || success == false){
        printf("表达式计算错误：%u, %s\n", result, buf);
        break;
        }
        printf("pass %u, %s\n", result, buf);
    }
    fclose(fp);
    return 0;
}

struct {
    const char *name;
    const char *description;
    int (*handler) (char *);
} cmd_table [] = {
    { "help", "Display information about all supported commands", cmd_help },
    { "c", "Continue the execution of the program", cmd_c },
    { "q", "Exit NEMU", cmd_q },

    /* TODO: Add more commands */
    {"si", "让程序单步执行N条指令后暂停执行,当N没有给出时, 缺省为1", cmd_si},
    {"info", "r: 打印寄存器状态 w: 打印监视点信息", cmd_info},
    {"x", "求出表达式EXPR的值, 将结果作为起始内存地址, 以十六进制形式输出连续的N个4字节", cmd_x},
    {"p", "求出表达式EXPR的值", cmd_p},
    {"w", "当表达式EXPR的值发生变化时, 暂停程序执行", cmd_w},
    {"d", "删除序号为N的监视点", cmd_d},
    {"test_expr", "使用gen_expr生成的文件测试expr功能", cmd_test_expr},
};
#define NR_CMD 10

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_loop() {
    char *read = NULL;

    while (true) {
        if (read) {
            free(read);
            read = NULL;
        }
        read = readline("npc> ");
        if (read == NULL || *read == '\0') {
            continue;
        }
        add_history(read);

        char *args = NULL;
        if (strchr(read, ' ') != NULL) {
            args = strchr(read, ' ') + 1;
        }
        char *cmd = strtok(read, " ");
            
        int i;
        for (i = 0; i < NR_CMD; i ++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                if (cmd_table[i].handler(args) < 0) { return; }
                break;
            }
        }
        if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
    }
}