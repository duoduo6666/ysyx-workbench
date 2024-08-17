#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define NR_WP 4

typedef uint32_t word_t;
word_t expr(char *e, bool *success);

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  char expr[32*32];
  word_t result;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

void free_wp(int no) {
  assert(no < NR_WP);
  wp_pool[no].next = free_;
  free_ = &wp_pool[no];
}

WP* new_wp(char* e) {
  WP* wp = free_;
  assert(wp != NULL);
  free_ = wp->next;
  wp->next = head;
  head = wp;
  bool success;
  strcpy(wp->expr, e);
  wp->result = expr(e, &success);
  if (!success) {printf("计算失败\n"); free_wp(wp->NO);;}
  return wp;
}

word_t expr(char *e, bool *success);
bool check_wp() {
  bool success;
  for (WP *wp = head; wp != NULL; wp = wp->next) {
    if (expr(wp->expr, &success) != wp->result) {
      wp->result = expr(wp->expr, &success);
      printf("触发监视点No.%d %s", wp->NO, wp->expr);
      return true;
    }
  }
  return false;
}

void display_wp() {
  for (WP *wp = head; wp != NULL; wp = wp->next) {
    printf("No.%d expr: %s value: %d", wp->NO, wp->expr, wp->result);
  }
}