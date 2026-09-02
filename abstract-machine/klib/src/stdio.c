#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static char *itoa(char *buf, int val) {
  char temp[32];
  int i = 0;
  
  // 1. 特殊处理 0
  if (val == 0) {
    buf[0] = '0';
    buf[1] = '\0'; // 关键！必须写 \0
    return buf;
  }

  // 2. 处理负数
  int is_neg = 0;
  unsigned int uval = val;
  if (val < 0) {
    is_neg = 1;
    uval = -val;
  }

  // 3. 逆序提取数字
  while (uval > 0) {
    temp[i++] = (uval % 10) + '0';
    uval /= 10;
  }

  // 4. 反转写入目标 buffer
  int j = 0;
  if (is_neg) {
    buf[j++] = '-';
  }
  while (i > 0) {
    buf[j++] = temp[--i];
  }
  buf[j] = '\0'; 

  return buf;
}

int printf(const char *fmt, ...) {
  char buf[2048]; // 临时缓冲区
  va_list args;
  va_start(args, fmt);
  int len = vsprintf(buf, fmt, args);
  va_end(args);

  for (int i = 0; i < len; i++) {
    putch(buf[i]); // putch 会自动写入 0x10000000 串口
  }
  return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *str = out;
  for (int i = 0; fmt[i] != '\0'; i++) {
    if (fmt[i] == '%') {
      i++;
      // 1. 检查是否有 '0' 标志（补零）
      int zero_pad = 0;
      if (fmt[i] == '0') {
        zero_pad = 1;
        i++;
      }
      // 2. 检查是否有指定宽度（如 2）
      int width = 0;
      while (fmt[i] >= '0' && fmt[i] <= '9') {
        width = width * 10 + (fmt[i] - '0');
        i++;
      }

      switch (fmt[i]) {
        case 'd': {
          int val = va_arg(ap, int);
          char num_buf[32];
          itoa(num_buf, val); // 先转成字符串
          int num_len = strlen(num_buf);
          
          // 如果需要补零/补空格
          while (width > num_len) {
            *str++ = zero_pad ? '0' : ' ';
            width--;
          }
          for (int j = 0; j < num_len; j++) {
            *str++ = num_buf[j];
          }
          break;
        }
        case 's': {
          char *s = va_arg(ap, char *);
          if (s == NULL) s = "(null)";
          while (*s != '\0') *str++ = *s++;
          break;
        }
        case 'c': {
          *str++ = (char)va_arg(ap, int);
          break;
        }
        default: {
          *str++ = fmt[i];
          break;
        }
      }
    } else {
      *str++ = fmt[i];
    }
  }
  *str = '\0';
  return str - out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int len = vsprintf(out, fmt, args);
  va_end(args);
  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
