#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <regex.h>
#include <string.h>
#include <assert.h>

#define MEMORY_SIZE 0x8000000
#define MEMORY_LEFT  0x80000000
#define MEMORY_RIGHT MEMORY_LEFT + MEMORY_SIZE

typedef uint32_t word_t;
word_t get_reg(char* reg, bool *success);
extern uint8_t memory[MEMORY_SIZE];

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_DEC, TK_OPCODE, TK_HEX, TK_REG
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
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

#define NR_REGEX 13

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_expr() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      printf("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
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

        // printf("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        if (substr_len >= 32) {
          printf("token过大");
          return false;
        }
        if (nr_token >= 32) {
          printf("token数量过多");
          return false;
        }
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

word_t eval(int p, int q, bool *success) {
  int ptr = 0;
  if (p > q) {
    printf("p > q");
    *success = false;
    return 0;
  }

  if (p == q) {
    word_t n = 0;
    switch (tokens[p].type){
      case TK_DEC:
        n = sscanf(tokens[p].str, "%d", &n);
        // n = atoi(tokens[p].str);
        break;
      case TK_HEX:
        sscanf(tokens[p].str+2, "%x", &n);
        break;
      case TK_REG:
        n = get_reg(tokens[p].str+1, success);
        if (!success) {printf("读取寄存器失败"); return 0;}
        break;
      default:
        printf("未知Token类型");
        *success = false;
        return 0;
    }
    return n;
  }
  // 负数
  else if (tokens[p].str[0] == '-' && (tokens[p+1].type == '(' && tokens[q].type == ')')) {
    return -eval(p+1, q, success);
  }
  else if (tokens[p].str[0] == '-' && q - p == 1) {
    return -eval(p+1, p+1, success);
  }
  else if (tokens[p].str[0] == '*' && (tokens[p+1].type == '(' && tokens[q].type == ')')) {
    ptr = eval(p+1, q, success);
    goto ptr_jump;
  }
  else if (tokens[p].str[0] == '*' && q - p == 1) {
    ptr = eval(p+1, p+1, success);
    ptr_jump:
    if ((MEMORY_LEFT > ptr) | (ptr+4-1 > MEMORY_RIGHT)) {printf("内存地址应在 0x%x-0x%x 之间", MEMORY_LEFT, MEMORY_RIGHT-4); return 0;}
    return *(word_t*)(&(memory[ptr-MEMORY_LEFT]));
  }
  else if (check_parentheses(p, q) == true){
    return eval(p + 1, q - 1, success);
  }
  else{
    int op = 0;
    int priority = 256;
    // 寻找主运算符
    for (int i = p; i < q; i++){
      // 跳过 '()'
      if (tokens[i].type == '(') {
        int t = i;
        for (; check_parentheses(t, i) == false; i++) {
          if (i >= 32) {
            *success = false;
            printf("表达式不合法");
            return 0;
          }
        }
      }
      if (i >= 32) {
        break;
      }

      if (tokens[i].type == TK_OPCODE || tokens[i].type == TK_EQ){
        if (tokens[i].str[0] == '+') {
          if (priority >= 10) {priority = 10; op = i;}
          continue;
        }
        else if ((tokens[i].str[0] == '-')) {
          // 如果这个减号在第一个token或者他的上一个token也是运算符就判定为负号
          if ((i == p) | (tokens[i-1].type == TK_OPCODE || tokens[i-1].type == '(')) {
            // 跳过这个负号
            continue;
          }
          if (priority >= 10) {priority = 10; op = i;}
          continue;
        }
        else if (tokens[i].str[0] == '*') {
          if ((i == p) | (tokens[i-1].type == TK_OPCODE || tokens[i-1].type == '(')) {
            continue;
          }
          if (priority >= 11) {priority = 11; op = i;}
          continue;
        }
        else if (tokens[i].str[0] == '/') {
          if (priority >= 11) {priority = 11; op = i;}
          continue;
        }
        else if (tokens[i].str[0] == '=' || tokens[i].str[0] == '!') {
          if (priority >= 9) {priority = 9; op = i;}
          continue;
        }
        else if (tokens[i].str[0] == '&') {
          if (priority >= 8) {priority = 8; op = i;}
          continue;
        }
        else {
          printf("未知运算符 %c", tokens[i].str[0]);
          assert(0);
        }
      }
    }

    int val1 = eval(p, op - 1, success);
    int val2 = eval(op + 1, q, success);
    // printf("v1 %u v2 %u op %d\n", val1, val2, op);
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
      if (val2 == 0) {printf("除数为0"); *success = false; return 0;}
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
      printf("未知运算符 %c", tokens[op].str[0]);
      assert(0);
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

  // for (int i = 0; i < 32; i++){
  //   printf("token[%d] = %d %s",
  //     i, tokens[i].type, tokens[i].str);
  // }

  int q;
  for (q=31; tokens[q].type == 0; q--){
    if (q < 0) { printf("空表达式"); assert(0); }
  }
  *success = true;
  return eval(0, q, success);
}
