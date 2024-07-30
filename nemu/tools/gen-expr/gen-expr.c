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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#define max_space 4
#define max_token 32

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"#include <stdint.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

char* gen_rand_space(char* start, int max) {
  if (max == 0) {
    return start;
  }
  int n = rand() % max;
  for (int i = 0; i < n; i++) {
    start[i] = ' ';
  }
  return start+n;
}

 char* gen_rand_expr(char* start, int max_token_num) {

  int token_num = 1;
  char* buf_ptr = start;
  bool bracket = false;
  buf_ptr = gen_rand_space(buf_ptr, max_space);
  // 是否生成负数                              gcc不能计算1--1
  if (max_token_num == 2 && rand() % 2 && *(buf_ptr-1) != '-') {
    *buf_ptr = '-';
    buf_ptr++;
    token_num++;
    buf_ptr = gen_rand_space(buf_ptr, max_space);
  }
  // 是否生成括号
  if (max_token_num >= 3 && rand() % 2) {
    bracket = true;
    *buf_ptr = '(';
    buf_ptr++;
    token_num += 2;
    buf_ptr = gen_rand_space(buf_ptr, max_space);
  }
  if (max_token_num-token_num <= 2 || (max_token_num <= 5 && bracket)) {
    buf_ptr += sprintf(buf_ptr, "%d", rand());
    buf_ptr = gen_rand_space(buf_ptr, max_space);
    return buf_ptr;
  }
  assert(max_token_num-token_num >= 3);
  int expr0_max_token_num = rand() % (max_token_num-token_num-1);
  if (expr0_max_token_num == 0) {
    expr0_max_token_num = 1;
  }
  int expr1_max_token_num = max_token_num-token_num-expr0_max_token_num;
  // printf("token %d e0 %d e1 %d\n", token_num, expr0_max_token_num, expr1_max_token_num);
  buf_ptr = gen_rand_expr(buf_ptr, expr0_max_token_num);
  buf_ptr = gen_rand_space(buf_ptr, max_space);

  switch (rand() % 4) {
    case 0:
      *buf_ptr = '+';
      break;
    case 1:
      *buf_ptr = '-';
      break;
    case 2:
      *buf_ptr = '*';
      break;
    case 3:
      *buf_ptr = '/';
      break;
    default:
      assert("Error");
  }
  buf_ptr++;

  buf_ptr = gen_rand_expr(buf_ptr, expr1_max_token_num);
  buf_ptr = gen_rand_space(buf_ptr, max_space);
  
  if (bracket) {
    *buf_ptr = ')';
    buf_ptr++;
  }
  
  *buf_ptr = '\0';
  return buf_ptr;
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i = 0;
  
  FILE *input = fopen("input", "w");
  assert(input != NULL);
  while (i < loop) {
    gen_rand_expr(buf, max_token);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -Wno-overflow -Wall -Werror -W /tmp/.code.c -o /tmp/.expr > /dev/null 2>&1");
    // int ret = system("gcc  -Wall -Werror -W /tmp/.code.c -o /tmp/.expr > /dev/null 2>&1");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    fprintf(input, "%u %s\n", result, buf);
    i++;
    printf("\r%d/%d", i, loop);
    fflush(stdout);
  }
  fclose(input);
  printf("\n");
  return 0;
}
