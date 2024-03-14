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
  WP *prev = NULL; // 前一个节点指针
  WP *curr = head; // 当前节点指针

  // 遍历查找编号匹配的监视点
  while (curr) {
    if (curr->NO == num) {
      break;
    }
    prev = curr;       // 更新前一个节点为当前节点
    curr = curr->next; // 移动到下一个节点
  }

  // 如果未找到，返回false
  if (curr == NULL) {
    return false;
  }

  // 如果目标监视点是头节点
  if (curr == head) {
    head = head->next; // 移动头指针
  } else {
    // 如果目标监视点是中间或尾节点
    prev->next = curr->next; // 跳过当前节点
  }

  // 将回收的节点加入到空闲链表的头部
  curr->next = free_;
  free_ = curr;

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
  WP *curr;
  curr = head;
  bool *success = malloc(1);
  while (curr != NULL) {
    uint32_t curr_val;
    curr_val = expr(curr->expr, success);
    if (curr->value != curr_val) {
      printf("%d号监测点表达式%s的值改变！\n", curr->NO, curr->expr);
      printf("旧值: %d  新值: %d\n\n", curr->value, curr_val);
      curr->value = curr_val;
      return false;
    }
    curr = curr->next;
  }
  return true;
}
