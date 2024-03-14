#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <sys/types.h>

enum {
  TK_NOTYPE = 256,
  TK_HEX,
  TK_DEC,
  TK_REG,
  TK_EQ,
  TK_NEQ,
  TK_AND,
  TK_OR,
  TK_RS,
  TK_LS,
  TK_GEQ,
  TK_LEQ,
  TK_DEREF,
  TK_NEG

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE},                     // spaces
    {"0x[0-9A-Fa-f][0-9A-Fa-f]*", TK_HEX}, // hex number
    {"0|[1-9][0-9]*", TK_DEC},             // dec number
    {"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip|ax|cx|dx|bx|sp|bp|si|di|al|cl|dl|"
     "bl|ah|ch|dh|bh)",
     TK_REG},
    {"\\+", '+'},      // add
    {"-", '-'},        // sub
    {"\\*", '*'},      // mul
    {"\\/", '/'},      // div
    {"\\(", '('},      // lparen
    {"\\)", ')'},      // rparen
    {"==", TK_EQ},     // equal
    {"!=", TK_NEQ},    // not equal
    {"&&", TK_AND},    // and
    {"\\|\\|", TK_OR}, // or
    {">>", TK_RS},     // right shift
    {"<<", TK_LS},     // left shift
    {"<=", TK_LEQ},    // less equal
    {">=", TK_GEQ},    // great equal
    {"<", '<'},        // less
    {">", '>'}         // great
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++) {
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

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i,
            rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
        case TK_NOTYPE:
          break;
        default:
          // 缓冲区大小设置为32
          if (substr_len >= 32) {
            printf("Token length is too long!\n");
            return false;
          }
          tokens[nr_token].type = rules[i].token_type;
          memset(tokens[nr_token].str, 0, 32);
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

// 检查括号
int check_parentheses(int p, int q) {
  // 边缘括号
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return 0;
  }
  int leftp = 0, rightp = 0;
  for (int i = p + 1; i < q; i++) {
    if (tokens[i].type == '(') {
      leftp++;
    } else if (tokens[i].type == ')') {
      if (leftp > rightp) {
        rightp++;
      } else {
        // 右括号多了，表达式出错
        return 0;
      }
    }
  }
  if (leftp == rightp) {
    return 1;
  } else {
    return 0;
  }
}

// 寻找主运算符
int get_domi_oper(int p, int q) {
  int pos = -1;            // 主运算符的位置
  int prior = 0;           // 当前最大的运算符优先级
  bool in_parentheses = 1; // 标记是否在括号内部

  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      in_parentheses = 0; // 进入括号内部
      continue;
    } else if (tokens[i].type == ')') {
      in_parentheses = 1; // 离开括号
      continue;
    }

    if (!in_parentheses) {
      continue; // 在括号中的运算符不考虑
    }

    int token_prior = 0; // 当前运算符的优先级

    // 判断运算符优先级
    switch (tokens[i].type) {
    case TK_NEG:   // 单目负号
    case TK_DEREF: // 解引用
    case '!':      // 逻辑非
      token_prior = 1;
      break;
    case '*': // 乘法
    case '/': // 除法
      token_prior = 2;
      break;
    case '+': // 加法
    case '-': // 减法
      token_prior = 3;
      break;
    case TK_EQ:  // 等于
    case TK_NEQ: // 不等于
      token_prior = 4;
      break;
    case TK_AND: // 逻辑与
    case TK_OR:  // 逻辑或
      token_prior = 5;
      break;
    default: // 其他情况不修改优先级
      break;
    }

    // 更新最大优先级的运算符位置
    if (token_prior >= prior) {
      prior = token_prior;
      pos = i;
    }
  }
  return pos; // 返回主运算符的位置
}

static int eval(int p, int q) { // 求值函数
  if (p > q) {
    assert(0); // 当p>q时，断言失败
  } else if (p == q) {
    // 单一元素求值
    int res;
    switch (tokens[q].type) {
    case TK_DEC:
      sscanf(tokens[q].str, "%d", &res);
      break;
    case TK_HEX:
      sscanf(tokens[q].str, "%x", &res);
      break;
    case TK_REG: {
      // 对寄存器求值
      if (strcmp(tokens[q].str, "eip") == 0) {
        return cpu.eip;
      }
      for (int i = 0; i < 8; i++) {
        if (strcmp(tokens[q].str, regsl[i]) == 0) {
          return reg_l(i);
        } else if (strcmp(tokens[q].str, regsw[i]) == 0) {
          return reg_w(i);
        } else if (strcmp(tokens[q].str, regsb[i]) == 0) {
          return reg_b(i);
        }
      }
      assert(0); // 未知寄存器
      break;
    }
    default:
      assert(0); // 未知类型
    }
    return res;
  } else if (check_parentheses(p, q) == 1) {
    // 去除边缘括号
    return eval(p + 1, q - 1);
  } else {
    // 处理表达式
    int pos = get_domi_oper(p, q);
    int val1, val2;

    if (tokens[pos].type == TK_NEG) {
      return -eval(p + 1, q);
    } else if (tokens[pos].type == TK_DEREF) {
      int addr = eval(p + 1, q);
      return vaddr_read(addr, 4);
    } else if (tokens[pos].type == '!') {
      return !eval(p + 1, q);
    }

    val1 = eval(p, pos - 1);
    val2 = eval(pos + 1, q);

    switch (tokens[pos].type) {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      if (val2 != 0) {
        return val1 / val2;
      } else {
        assert(0); // 防止除以0
      }
    case TK_EQ:
      return val1 == val2;
    case TK_NEQ:
      return val1 != val2;
    case TK_AND:
      return val1 && val2;
    case TK_OR:
      return val1 || val2;
    default:
      assert(0); // 未知运算符
    }
  }
  return -1; // 表达式无法求值
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '-') { // 负号的处理
      if (i == 0 ||
          (tokens[i - 1].type != TK_DEC && tokens[i - 1].type != TK_HEX &&
           tokens[i - 1].type != TK_REG && tokens[i - 1].type != ')'))
        tokens[i].type = TK_NEG;
    } else if (tokens[i].type == '*') { // 解引用的处理，同负号
      if (i == 0 ||
          (tokens[i - 1].type != TK_DEC && tokens[i - 1].type != TK_HEX &&
           tokens[i - 1].type != TK_REG && tokens[i - 1].type != ')'))
        tokens[i].type = TK_DEREF;
    }
  }
  int value = eval(0, nr_token - 1);
  return value;
}
