#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  // 被监视的表达式和值
  char expr[32]; // 表达式长度有限制
  int value;

} WP;

WP *new_wp();
bool free_wp(int num);
void print_wp();
bool watch_wp();

#endif
