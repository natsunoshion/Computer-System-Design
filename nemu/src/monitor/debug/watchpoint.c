#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];

// 分配链表和空闲链表
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

WP *new_wp() {
  WP *temp;
  if (free_ == NULL) {
    // 没有空闲的WP，返回空
    return NULL;
  }
  temp = free_;
  free_ = free_->next;
  temp->next = head;
  head = temp;
  temp->value = 0;
  return temp;
}

bool free_wp(int num) { return true; }

/* TODO: Implement the functionality of watchpoint */
void print_wp() {
  if (head == NULL) {
    printf("暂无监视点！\n");
    return;
  }
  printf("监视点：\n");
  WP *curr = head;
  while (curr) {
    printf("%d  %s: %d\n", curr->NO, curr->expr, curr->value);
    curr = curr->next;
  }
}
