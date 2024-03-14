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

int get_operator_priority(int type) {
  switch (type) {
  case TK_NOTYPE:
  case ')':
    return 0;
  case TK_OR:
    return 1;
  case TK_AND:
    return 2;
  case TK_EQ:
  case TK_NEQ:
    return 3;
  case '<':
  case '>':
  case TK_LEQ:
  case TK_GEQ:
    return 4;
  case TK_LS:
  case TK_RS:
    return 5;
  case '+':
  case '-':
    return 6;
  case '*':
  case '/':
    return 7;
  case TK_NEG:
  case TK_DEREF:
    return 8;
  case '(':
    return 9;
  default:
    printf("未知的操作符类型！\n");
    assert(0);
  }
}

bool is_number_token(int type) {
  return type == TK_DEC || type == TK_HEX || type == TK_REG;
}

int get_token_value(Token token) {
  int ret_value = 0;
  switch (token.type) {
  case TK_DEC:
    sscanf(token.str, "%d", &ret_value);
    break;
  case TK_HEX:
    sscanf(token.str, "%x", &ret_value);
    break;
  case TK_REG: {
    char reg[4] = {0};
    sscanf(token.str, "$%s", reg);
    for (int i = 0; i < 8; i++) {
      if (strcasecmp(reg, regsl[i]) == 0) {
        return cpu.gpr[i]._32;
      } else if (strcasecmp(reg, regsw[i]) == 0) {
        return cpu.gpr[i]._16;
      } else if (strcasecmp(reg, regsb[i]) == 0) {
        return cpu.gpr[i % 4]._8[i / 4];
      } else if (strcasecmp(reg, "eip") == 0) {
        return cpu.eip;
      }
    }
    printf("非法寄存器！\n");
    assert(0);
    break;
  }
  default:
    printf("非法的值标记！\n");
    assert(0);
    break;
  }
  return ret_value;
}

int eval(int start, int end, bool *success) {
  // 若起点大于终点，表达式错误
  if (start > end) {
    printf("表达式错误！\n");
    *success = false;
    return 0;
  }
  // 只有一个标记
  if (start == end) {
    printf("%d %s\n", tokens[start].type, tokens[start].str);
    return get_token_value(tokens[start]);
  } else {
    tokens[++end].type = TK_NOTYPE; // 在末尾添加一个无类型标记
    // 操作数栈和操作符栈
    int num_stack[32];
    Token op_stack[32];
    int num_top = -1, op_top = 0;
    op_stack[0].type = TK_NOTYPE; // 操作符栈初始值
    for (int i = start; i <= end; i++) {
      // 若标记是数字，则压入操作数栈
      if (is_number_token(tokens[i].type)) {
        num_stack[++num_top] = get_token_value(tokens[i]);
      }
      // 若标记是操作符
      else {
        // 检查栈顶操作符优先级
        // 如果当前操作符优先级大于栈顶操作符，则压入操作符栈
        if (get_operator_priority(tokens[i].type) >
            get_operator_priority(op_stack[op_top].type)) {
          if (tokens[i].type == '(') {
            op_stack[++op_top].type = ')';
          } else {
            op_stack[++op_top] = tokens[i];
          }
        }
        // 否则，计算临时结果
        else {
          while (op_top != 0 &&
                 get_operator_priority(tokens[i].type) <=
                     get_operator_priority(op_stack[op_top].type)) {
            if (op_stack[op_top].type == ')') {
              op_top--;
              break;
            } else {
              switch (op_stack[op_top--].type) {
              case '+':
                num_stack[num_top - 1] =
                    num_stack[num_top] + num_stack[num_top - 1];
                num_top--;
                break;
              case '-':
                num_stack[num_top - 1] =
                    num_stack[num_top] - num_stack[num_top - 1];
                num_top--;
                break;
              case '*':
                num_stack[num_top - 1] =
                    num_stack[num_top] * num_stack[num_top - 1];
                num_top--;
                break;
              case '/':
                num_stack[num_top - 1] =
                    num_stack[num_top] / num_stack[num_top - 1];
                num_top--;
                break;
              case '<':
                num_stack[num_top - 1] =
                    num_stack[num_top] < num_stack[num_top - 1];
                num_top--;
                break;
              case '>':
                num_stack[num_top - 1] =
                    num_stack[num_top] > num_stack[num_top - 1];
                num_top--;
                break;
              case TK_EQ:
                num_stack[num_top - 1] =
                    num_stack[num_top] == num_stack[num_top - 1];
                num_top--;
                break;
              case TK_NEQ:
                num_stack[num_top - 1] =
                    num_stack[num_top] != num_stack[num_top - 1];
                num_top--;
                break;
              case TK_AND:
                num_stack[num_top - 1] =
                    num_stack[num_top] && num_stack[num_top - 1];
                num_top--;
                break;
              case TK_OR:
                num_stack[num_top - 1] =
                    num_stack[num_top] || num_stack[num_top - 1];
                num_top--;
                break;
              case TK_LS:
                num_stack[num_top - 1] = num_stack[num_top]
                                         << num_stack[num_top - 1];
                num_top--;
                break;
              case TK_RS:
                num_stack[num_top - 1] =
                    num_stack[num_top] >> num_stack[num_top - 1];
                num_top--;
                break;
              case TK_LEQ:
                num_stack[num_top - 1] =
                    num_stack[num_top] <= num_stack[num_top - 1];
                num_top--;
                break;
              case TK_GEQ:
                num_stack[num_top - 1] =
                    num_stack[num_top] >= num_stack[num_top - 1];
                num_top--;
                break;
              case TK_NEG:
                num_stack[num_top] = -num_stack[num_top];
                break;
              case TK_DEREF:
                num_stack[num_top] = paddr_read(num_stack[num_top], 4);
                break;
              default:
                *success = false;
                return 0;
              }
            }
          }
          if (tokens[i].type != ')' && tokens[i].type != TK_NOTYPE) {
            op_stack[++op_top] = tokens[i];
          }
        }
      }
    }
    if (op_top != 0) {
      *success = false;
      return 0;
    }
    return num_stack[0];
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // 处理单目运算
  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '*' || tokens[i].type == '-') {
      if (i == 0 ||
          (tokens[i - 1].type != TK_DEC && tokens[i - 1].type != TK_HEX &&
           tokens[i - 1].type != TK_REG && tokens[i - 1].type != ')')) {
        tokens[i].type = tokens[i].type == '*' ? TK_DEREF : TK_NEG;
      }
    }
  }
  int value = eval(0, nr_token - 1, success);
  return value;
}
