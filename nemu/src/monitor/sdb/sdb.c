/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <utils.h>

#include <stdlib.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
word_t expr(char *e, bool *success);

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[32*32];
  word_t result;
} WP;
WP* new_wp(char* expr);
void free_wp(int no);
void display_wp();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  set_nemu_state(NEMU_QUIT, isa_reg_pc(), 0);
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
  char *endptr;
  long num = 1;
  if (args) {num = strtol(args, &endptr, 10);}
  cpu_exec(num);
  return 0;
}

static int cmd_info(char *args) {
  if (!args) {Log("r: 打印寄存器状态, w: 打印监视点信息"); return 1;}
  if (args[0] == 'r') {isa_reg_display();}
  if (args[0] == 'w') {display_wp();}
  return 0;
}

static int cmd_x(char *args) {
  if (!args) {Log("x N EXPR 求出表达式EXPR的值, 将结果作为起始内存地址, 以十六进制形式输出连续的N个4字节"); return 1;}
  int num = 0;
  int ptr = 0;
  sscanf(args, "%d %x", &num, &ptr);
  if ((PMEM_LEFT > ptr) | (ptr+num*4-1 > PMEM_RIGHT)) {Log("内存地址应在 0x%x-0x%x 之间", PMEM_LEFT, PMEM_RIGHT); return 1;}
  for (int i = 0; i < num; i++){
    printf("0x%x: 0x%x\n", ptr+i*4, paddr_read(ptr+i*4, 4));
  }
  return 0;
}

static int cmd_p(char *args) {
  if (!args) {Log("p EXPR 求出表达式EXPR的值"); return 1;}
  bool success;
  word_t result = expr(args, &success);
  if (success) {printf("%u\n", result);}
  else {printf("计算失败\n");}
  return 0;
}

static int cmd_w(char *args) {
  if (!args) {Log("w EXPR 当表达式EXPR的值发生变化时, 暂停程序执行"); return 1;}
  WP* wp = new_wp(args);
  printf("watch point NO.%d\n", wp->NO);
  return 0;
}

static int cmd_d(char *args) {
  if (!args) {Log("d N 删除序号为N的监视点"); return 1;}
  int n = atoi(args);
  free_wp(n);
  return 0;
}

static int cmd_test_expr(char *args) {
  if (!args) {Log("test_expr filename"); return 1;}

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

static struct {
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

#define NR_CMD ARRLEN(cmd_table)

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

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

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

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
