/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char *expr;
  int val;
  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    /* the id is set and will never change */
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].val = 0;
    wp_pool[i].expr = "\0";
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
/* 申请并启用一个空闲的监视点 */
WP* new_wp() {
  if (free_ == NULL) {
    printf("Error: No vacant watchpoint left!\n");
    assert(0);
  }

  WP *wp = free_;
  free_ = free_->next;

  wp->next = head;
  head = wp;

  return wp;
}

/* 释放一个正在使用的监视点，将其归还到空闲链表 */
void free_wp(WP *wp) {
  assert(wp != NULL);

  if (head == wp) {
    head = head->next;
  } else {
    WP *prev = head;
    while (prev && prev->next != wp) {
      prev = prev->next;
    }
    if (prev == NULL) {
      printf("Error: This watchpoint is not in the active list!\n");
      assert(0);
    }
    prev->next = wp->next;
  }

  wp->next = free_;
  free_ = wp;

  wp->expr[0] = '\0';
  wp->val = 0;
}

// 在 watchpoint.c 中实现
bool check_watchpoints() {
  bool triggered = false;
  WP *ptr = head;
  while (ptr != NULL) {
    bool success = false;
    word_t new_val = expr(ptr->expr, &success);
    if (!success) {
      printf("Warning: Failed to evaluate watchpoint %d expression: %s\n", ptr->NO, ptr->expr);
      ptr = ptr->next;
      continue;
    }
    if (new_val != ptr->val) {
      printf("\nWatchpoint %d triggered: %s\n", ptr->NO, ptr->expr);
      printf("  Old value = %u (0x%08x)\n", ptr->val, ptr->val);
      printf("  New value = %u (0x%08x)\n", new_val, new_val);
      ptr->val = new_val;
      triggered = true;
    }
    ptr = ptr->next;
  }
  return triggered;
}

bool add_watchpoint(char *expr_str) {
    bool success = false;
    word_t val = expr(expr_str, &success);
    if (!success) {
        printf("Invalid expression: %s\n", expr_str);
        return false;
    }
    WP *wp = new_wp(); // 内部调用
    strcpy(wp->expr, expr_str);
    wp->val = val;
    printf("Set watchpoint %d: %s\n", wp->NO, wp->expr);
    return true;
}

bool delete_watchpoint(int no) {
    // 遍历 head 找到 NO == no 的监视点 wp
    WP* ptr = head;
    while(ptr && ptr->NO != no) ptr = ptr->next;
    if(!ptr) {
      printf("The watchpoint number unfound in the active list!\n");
      return false;
    }
    // 然后调用 free_wp(wp)
    free_wp(ptr);
    return true;
}

void watchpoint_display() {
  printf("Num Type Disp Enb Value What");
  WP* ptr = head;
  while(ptr) {
    printf("%d watchpoint keep %d %s", ptr->NO, ptr->val, ptr->expr);
  }
}