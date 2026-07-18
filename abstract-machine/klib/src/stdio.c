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
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char* str = out;
  for(int i = 0; fmt[i] != '\0'; i++) {
    if(fmt[i] == '%') {
      i++;
      switch (fmt[i]) {
        case 's': {
          char *s = va_arg(args, char *);
          if (s == NULL) {
            s = "(null)";
          }
          while (*s != '\0') {
            *str++ = *s++;
          }
          break;
        }
        case 'd': {
          int val = va_arg(args, int);
          str = itoa(str, val); // 直接在当前位置写入数字
          break;
        }
      }
    }
    else {
      *str++ = fmt[i];
    }
  }
  *str = '\0';
  va_end(args);
  return str - out;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
