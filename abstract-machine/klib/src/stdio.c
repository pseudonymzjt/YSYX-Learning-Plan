#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static char *itoa(char *p, int num) {
  if (num == 0) {
    *p++ = '0';
    return p;
  }

  char temp[32];
  int i = 0;
  unsigned int abs_num = num;

  if (num < 0) {
    *p++ = '-';
    abs_num = -num; 
  }
  while (abs_num > 0) {
    temp[i++] = (abs_num % 10) + '0';
    abs_num /= 10;
  }
  while (i > 0) {
    *p++ = temp[--i];
  }

  return p;
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
      switch (fmt[i]) {
        case 's': {
          char *s = va_arg(ap, char *);
          if (s == NULL) s = "(null)";
          while (*s != '\0') {
            *str++ = *s++;
          }
          break;
        }
        case 'd': {
          int val = va_arg(ap, int);
          str = itoa(str, val); // 直接在当前位置写入数字
          break;
        }
        case 'c': {
          char c = (char)va_arg(ap, int);
          *str++ = c;
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
