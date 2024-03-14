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

int eval(int p, int q, bool *success) { return 0; }

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
