#include <isa.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

/* 我们使用 POSIX regex 函数来处理正则表达式 */
#include <regex.h>

// 声明外部内存读取接口，用于指针解引用
word_t vaddr_read(vaddr_t addr, int len);

enum {
  TK_NOTYPE = 256, 
  TK_EQ,
  TK_NOTEQ,   // !=
  TK_AND,     // &&
  TK_OR,      // ||
  TK_DEC,     // 十进制整数
  TK_HEX,     // 十六进制整数 (如 0x80100000)
  TK_REG,     // 寄存器 (如 $eax, $eip)
  TK_DEREF,   // 指针解引用 *
  TK_NEG      // 单目负号 -
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  /* 注意：规则的定义顺序决定了匹配优先级。
   * 复杂算子（如 ==, !=）应置于基础算子（如 +, -, *, /）之前。
   * 十六进制应置于十进制之前，避免 "0x..." 前部的 "0" 被十进制规则截断。
   */
  {" +", TK_NOTYPE},             // 空格
  {"==", TK_EQ},                 // 等于
  {"!=", TK_NOTEQ},              // 不等于
  {"&&", TK_AND},                // 逻辑与
  {"\\|\\|", TK_OR},             // 逻辑或
  {"\\+", '+'},                  // 加
  {"\\-", '-'},                  // 减
  {"\\*", '*'},                  // 乘
  {"\\/", '/'},                  // 除
  {"\\(", '('},                  // 左括号
  {"\\)", ')'},                  // 右括号
  {"0[xX][0-9a-fA-F]+", TK_HEX},  // 十六进制数
  {"[0-9]+", TK_DEC},            // 十进制数
  {"\\$[a-zA-Z0-9]+", TK_REG}    // 寄存器名
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
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


static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* 挨个尝试所有的规则 */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        // 如果是空格，不记录到 tokens 数组中
        if (rules[i].token_type != TK_NOTYPE) {
          if (nr_token >= ARRLEN(tokens)) {
            printf("Too many tokens in the expression.\n");
            return false;
          }

          tokens[nr_token].type = rules[i].token_type;

          switch (rules[i].token_type) {
            case TK_DEC:
            case TK_HEX:
            case TK_REG: {
              if (substr_len >= 32) {
                printf("Token length exceeds buffer size.\n");
                return false;
              }
              strncpy(tokens[nr_token].str, substr_start, substr_len);
              tokens[nr_token].str[substr_len] = '\0'; // 确保以 \0 结尾
              break;
            }
            default:
              break;
          }
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

/* 辅助函数 1：检查括号是否匹配，并且是否包围整个表达式 */
bool check_parentheses(int p, int q, bool *success) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }
  
  int count = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      count++;
    } else if (tokens[i].type == ')') {
      count--;
    }
    // 如果在未扫完整个表达式（即 i < q）之前，括号层数就已经回到了 0，
    // 说明最外层并不是一对相互匹配的括号。例如：(1+2)*(3+4)
    if (count == 0 && i < q) {
      return false;
    }
    if (count < 0) { // 遇到不合法的右括号
      *success = false;
      return false;
    }
  }
  
  if (count != 0) {
    *success = false; // 括号本身没有完全配对
    return false;
  }
  return true;
}

/* 辅助函数 2：获取运算符的结合优先级 */
int get_precedence(int type) {
  switch (type) {
    case TK_OR:    return 1; // 优先级最低
    case TK_AND:   return 2;
    case TK_EQ:
    case TK_NOTEQ: return 3;
    case '+':
    case '-':      return 4;
    case '*':
    case '/':      return 5; // 优先级最高
    default:       return -1; // 非双目运算符
  }
}

/* 辅助函数 3：在指定范围内寻找主运算符 (Dominant Operator) */
int get_dominant_op(int p, int q) {
  int op = -1;
  int min_prec = 999;
  int parentheses_count = 0;

  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') {
      parentheses_count++;
    } else if (tokens[i].type == ')') {
      parentheses_count--;
    } else if (parentheses_count == 0) {
      int prec = get_precedence(tokens[i].type);
      if (prec != -1) {
        // 双目运算符为左结合：当优先级相同时，应选取“最右边”的一个作为主运算符。
        // 因此使用 '<=' 来保证取到最右边优先级最低的运算符。
        if (prec <= min_prec) {
          min_prec = prec;
          op = i;
        }
      }
    }
  }
  return op;
}

/* 递归求值核心逻辑 */
word_t eval(int p, int q, bool *success) {
  if (!*success) return 0;

  if (p > q) {
    *success = false;
    return 0;
  } 
  else if (p == q) {
    /* 递归基：单个 Token */
    word_t val = 0;
    if (tokens[p].type == TK_DEC) {
      val = strtoul(tokens[p].str, NULL, 10);
    } 
    else if (tokens[p].type == TK_HEX) {
      val = strtoul(tokens[p].str, NULL, 16);
    } 
    else if (tokens[p].type == TK_REG) {
      bool reg_ok = false;
      // 寄存器匹配出来的字符串中包含开头的 '$'，应传给 isa_reg_str2val 时向后移动 1 位偏移以跳过 '$'
      val = isa_reg_str2val(tokens[p].str + 1, &reg_ok); 
      if (!reg_ok) {
        printf("Invalid register: %s\n", tokens[p].str);
        *success = false;
        return 0;
      }
    } 
    else {
      *success = false;
      return 0;
    }
    return val;
  } 
  else if (check_parentheses(p, q, success) == true) {
    /* 脱去外层括号 */
    return eval(p + 1, q - 1, success);
  } 
  else {
    if (!*success) return 0;

    int op = get_dominant_op(p, q);
    
    // 如果没有找到双目主运算符，但两端不匹配，可能存在单目运算符（如负数 -5 或解引用 *0x80000000）
    if (op == -1) {
      if (tokens[p].type == TK_NEG) {
        return -eval(p + 1, q, success);
      } 
      else if (tokens[p].type == TK_DEREF) {
        word_t addr = eval(p + 1, q, success);
        return vaddr_read(addr, 4);
      } 
      else {
        *success = false;
        return 0;
      }
    }

    // 分治：递归求解左右子表达式
    word_t val1 = eval(p, op - 1, success);
    word_t val2 = eval(op + 1, q, success);

    if (!*success) return 0;

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/':
        if (val2 == 0) {
          printf("Runtime error: Division by zero!\n");
          *success = false;
          return 0;
        }
        return val1 / val2;
      case TK_EQ:    return val1 == val2;
      case TK_NOTEQ: return val1 != val2;
      case TK_AND:   return val1 && val2;
      case TK_OR:    return val1 || val2;
      default:
        *success = false;
        return 0;
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* 核心预处理：区分单目运算符与双目运算符
   * 词法分析时我们无法区分 '*' 是乘法还是指针解引用，'-' 是减法还是负号。
   * 通过观察其前驱 Token 的属性来完成纠偏：
   * 如果星号/减号是第一个 Token，或者它前面的 Token 既不是数字、不是寄存器、也不是右括号，
   * 那么它一定是个单目运算符。
   */
  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && (i == 0 || (
        tokens[i - 1].type != TK_DEC && 
        tokens[i - 1].type != TK_HEX && 
        tokens[i - 1].type != TK_REG && 
        tokens[i - 1].type != ')'))) {
      tokens[i].type = TK_DEREF;
    }
    if (tokens[i].type == '-' && (i == 0 || (
        tokens[i - 1].type != TK_DEC && 
        tokens[i - 1].type != TK_HEX && 
        tokens[i - 1].type != TK_REG && 
        tokens[i - 1].type != ')'))) {
      tokens[i].type = TK_NEG;
    }
  }

  *success = true;
  word_t res = eval(0, nr_token - 1, success);
  
  if (!*success) {
    return 0;
  }
  return res;
}