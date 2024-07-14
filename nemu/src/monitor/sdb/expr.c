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
#include <memory/paddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_DEC, TK_OPCODE, TK_HEX, TK_REG
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {"0x[0-9abcdef]+", TK_HEX},
  {"[$].+", TK_REG},
  {"[0-9]+", TK_DEC},
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_OPCODE},         // plus
  {"\\-", TK_OPCODE},         // sub
  {"\\*", TK_OPCODE},         // *
  {"\\/", TK_OPCODE},         // /
  {"\\(", '('},         // *
  {"\\)", ')'},         // /
  {"==", TK_EQ},        // equal
  {"!=", TK_EQ},        
  {"&&", TK_OPCODE},        
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        Assert(substr_len < 32, "token过大");
        Assert(nr_token <= 32, "token数量过多");
        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          default: 
            tokens[nr_token].type = rules[i].token_type; 
            strncpy(tokens[nr_token].str, substr_start, substr_len); 
            tokens[nr_token].str[substr_len] = 0;
            nr_token++;
            break;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  for (;nr_token < 32; nr_token++) {
    tokens[nr_token].type = 0;
  }
  return true;
}

bool check_parentheses(int p, int q) {
  int deep = 0;
  if ((tokens[p].type != '(') | (tokens[q].type != ')')) {
    return false;
  }
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      deep++;
    }
    if (tokens[i].type == ')') {
      if ((deep == 1) & (i != q)) {
        return false;
      }
      deep--;
    }
    if (deep < 0) {
      return false;
    }
  }
  if (deep == 0) {
    return true;
  }
  return false;
}

word_t eval(int p, int q) {
  int ptr = 0;
  bool success;
  Assert(p <= q, "p > q");
  if (p == q) {
    word_t n = 0;
    switch (tokens[p].type){
      case TK_DEC:
        n = atoi(tokens[p].str);
        break;
      case TK_HEX:
        sscanf(tokens[p].str+2, "%x", &n);
        break;
      case TK_REG:
        n = isa_reg_str2val(tokens[p].str+1, &success);
        if (!success) {Log("读取寄存器失败");}
        break;
      default:
        Assert(0, "未知Token类型");
    }
    return n;
  }
  // 负数
  else if (tokens[p].str[0] == '-' && (tokens[p+1].type == '(' && tokens[q].type == ')')) {
    return -eval(p+1, q);
  }
  else if (tokens[p].str[0] == '-' && q - p == 1) {
    return -eval(p+1, p+1);
  }
  else if (tokens[p].str[0] == '*' && (tokens[p+1].type == '(' && tokens[q].type == ')')) {
    ptr = eval(p+1, q);
    goto ptr_jump;
  }
  else if (tokens[p].str[0] == '*' && q - p == 1) {
    ptr = eval(p+1, p+1);
    ptr_jump:
    if ((PMEM_LEFT > ptr) | (ptr+4-1 > PMEM_RIGHT)) {Log("内存地址应在 0x%x-0x%x 之间", PMEM_LEFT, PMEM_RIGHT-4); return 0;}
    return paddr_read(ptr, 4);
  }
  else if (check_parentheses(p, q) == true){
    return eval(p + 1, q - 1);
  }
  else{
    int op = 0;
    int priority = 256;
    // 寻找主运算符
    for (int i = p; i < q; i++){
      // 跳过 '()'
      if (tokens[i].type == '(') {
        for (; tokens[i].type != ')'; i++) {Assert(i < 32, "表达式不合法");}
      }
      Assert(i < 32, "表达式不合法");
      if (tokens[i].type == TK_OPCODE || tokens[i].type == TK_EQ){
        if (tokens[i].str[0] == '+') {
          if (priority > 10) {priority = 10; op = i;}
          continue;
        }
        else if ((tokens[i].str[0] == '-')) {
          // 如果这个减号在第一个token或者他的上一个token也是运算符就判定为负号
          if ((i == 0) | (tokens[i-1].type == TK_OPCODE)) {
            // 跳过这个负号
            continue;
          }
          if (priority > 10) {priority = 10; op = i;}
          continue;
        }
        else if (tokens[i].str[0] == '*') {
          if ((i == 0) | (tokens[i-1].type == TK_OPCODE)) {
            continue;
          }
          if (priority > 11) {priority = 11; op = i;}
          continue;
        }
        else if (tokens[i].str[0] == '/') {
          if (priority > 11) {priority = 11; op = i;}
          continue;
        }
        else if (tokens[i].str[0] == '=' || tokens[i].str[0] == '!') {
          if (priority > 9) {priority = 9; op = i;}
          continue;
        }
        else if (tokens[i].str[0] == '&') {
          if (priority > 8) {priority = 8; op = i;}
          continue;
        }
        else {
          Assert(0, "未知运算符 %c", tokens[i].str[0]);
        }
      }
    }

    int val1 = eval(p, op - 1);
    int val2 = eval(op + 1, q);
    switch (tokens[op].str[0]){
    case '+':
      return val1 + val2;
      break;
    case '-':
      return val1 - val2;
      break;
    case '*':
      return val1 * val2;
      break;
    case '/':
      if (val2 == 0) {Assert(0, "除数为0");}
      return val1 / val2;
      break;
    case '&':
      return val1 && val2;
      break;
    case '=':
      return val1 == val2;
      break;
    case '!':
      return val1 != val2;
      break;
    
    default:
      Assert(0, "未知运算符 %c", tokens[op].str[0]);
      break;
    }
  }
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // for (int i = 0; i < 32; i++){
  //   Log("token[%d] = %d %s",
  //     i, tokens[i].type, tokens[i].str);
  // }

  int q;
  for (q=31; tokens[q].type == 0; q--){
    Assert(q >= 0, "空表达式");
  }

  return eval(0, q);
}
