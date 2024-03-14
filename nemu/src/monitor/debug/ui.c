#include "monitor/expr.h"
#include "monitor/monitor.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin.
 */
char *rl_gets() {
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

static int cmd_q(char *args) { return -1; }

static int cmd_help(char *args);

static int cmd_si(char *args) {
  // 步数默认为1
  uint64_t N = 1;

  // 如果有参数，那就把步数拷贝到NN
  if (args != NULL) {
    if (sscanf(args, "%llu", &N) <= 0) {
      printf("参数非法！\n");
      return 0;
    }
  }

  // 调用api执行N步
  cpu_exec(N);
  return 0;
}

static int cmd_info(char *args) {
  char SUBCMD;
  if (args == NULL || sscanf(args, "%c", &SUBCMD) <= 0) {
    printf("参数非法！\n");
    return 0;
  }
  bool reg = SUBCMD == 'r';
  bool watchpoint = SUBCMD == 'w';
  if (!reg && !watchpoint) {
    printf("参数非法！\n");
    return 0;
  }
  if (reg) {
    // 寄存器
    // 32bit
    for (int i = 0; i < 8; i++) {
      printf("%s: 0x%x\n", regsl[i], reg_l(i)); // 16进制
    }

    // 16bit
    for (int i = 0; i < 8; i++) {
      printf("%s: 0x%x\n", regsw[i], reg_w(i));
    }

    // 8bit
    for (int i = 0; i < 8; i++) {
      printf("%s: 0x%x\n", regsb[i], reg_b(i));
    }
    printf("eip: 0x%x\n", cpu.eip); // eip
  } else if (watchpoint) {
    // 监视点
    print_wp();
  }
  return 0;
}

static int cmd_p(char *args) {
  bool ok;
  int value = expr(args, &ok);

  if (ok) {
    printf("%d\n", value);
  } else {
    printf("语法错误！\n");
  }

  return 0;
}

static int cmd_x(char *args) {
  if (args == NULL) {
    printf("参数非法！\n");
    return 0;
  }
  int N = atoi(strtok(args, " ")); // 要检查的字节数
  vaddr_t memory_address;          // 开始检查的内存地址
  // char *num_bytes_str = strtok(args, " ");
  char *address_str = strtok(NULL, " ");

  if (sscanf(args, "%d 0x%x", &N, &memory_address) <= 0) {
    printf("参数非法！\n");
    return 0;
  }

  bool valid_expr;
  memory_address =
      expr(address_str, &valid_expr); // 计算表达式的值来获取内存地址
  if (!valid_expr) {
    printf("表达式语法错误！\n");
    return 0;
  }
  printf("0x%x后%d个4字节的内存：\n", memory_address, N);
  for (int i = 0; i < N; i++) {
    uint32_t value = paddr_read(memory_address, 4);
    printf("0x%x : 0x%08x\n", memory_address, value);
    memory_address += 4;
  }
  printf("\n");

  return 0;
}

static int cmd_w(char *args) {
  bool ok;
  WP *wp = new_wp();
  strcpy(wp->expr, args);
  wp->value = expr(args, &ok);
  if (!ok) {
    printf("表达式语法错误！\n");
    return 0;
  } else {
    printf("成功将%s设置到%d号监视点\n", args, wp->NO);
  }
  return 0;
}

static int cmd_d(char *args) {
  int num = 0;
  if (sscanf(args, "%d", &num) <= 0) {
    printf("参数非法！\n");
    return 0;
  }

  if (free_wp(num)) {
    printf("成功删除监视点%d\n", num);
  } else {
    printf("无法找到监视点%d！\n", num);
  }
  return 0;
}

static struct {
  char *name;
  char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display informations about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {"si", "si [N]：单步执行N条指令，N可省，默认执行一条", cmd_si},
    {"info", "info r：打印寄存器状态\n       info w：打印监视点信息", cmd_info},
    {"p", "p EXPR：求出表达式EXPR（基础算术运算）的值", cmd_p},
    {"x",
     "x N EXPR：扫描内存，即求出EXPR的值，然后从EXPR地址上输出N个4字节数据",
     cmd_x},
    {"w", "w EXPR：当EXPR的值发生变化时，暂停程序", cmd_w},
    {"d", "d N：删除N号监视点", cmd_d},

    /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  } else {
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD) {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}
