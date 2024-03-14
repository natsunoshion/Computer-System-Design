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

/* TODO: Implement the functionality of watchpoint */
WP *new_wp() {
  WP *curr;
  if (free_ == NULL) {
    // 没有空闲的WP，返回空
    return NULL;
  }

  // 每次都是插入到头部，避免一些复杂的if判断
  curr = free_;
  free_ = free_->next;
  curr->next = head;
  head = curr;
  return curr;
}

bool free_wp(int num) {
  WP *curr = head;
  while (curr) {
    if (curr->NO == num) {
      break;
    }
    curr = curr->next;
  }
  if (curr == NULL) {
    return false;
  }
  // 回收头节点
  if (curr == head) {
    head = head->next;

    // 放进空闲链表
    curr->next = free_;
    free_ = curr;
  }
  // 回收中间节点
  else {
    curr = head;
    while (curr->next) {
      if (curr->next->NO == num) {
        break;
      }
      curr = curr->next;
    }
    WP *temp = curr->next;
    curr->next = temp->next;
    temp->next = free_;
    free_ = temp;
  }
  return true;
}

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

bool watch_wp() {
  WP *temp;
  temp = head;
  bool *success = malloc(1);
  while (temp != NULL) {
    uint32_t curr_val;
    curr_val = expr(temp->expr, success);
    if (temp->value != curr_val) {
      printf("%d号监测点表达式%s的值改变！\n", temp->NO, temp->expr);
      printf("旧值: %d  新值: %d\n\n", temp->value, curr_val);
      temp->value = curr_val;
      return false;
    }
    temp = temp->next;
  }
  return true;
}
