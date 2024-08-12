#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

size_t intToStr(int num, char* str) {
    size_t i = 0;

    // 负数
    if (num < 0) {
        str[i] = '-';
        i++;
        num = -num;
    }

    for (; num != 0; i++) {
        str[i] = (num % 10) + '0';
        num /= 10;
    }

    // 反转数字部分
    int start = str[0] == '-';
    int end = i - 1;
    while (start < end) {
        char t = str[start];
        str[start] = str[end];
        str[end] = t;
        start++;
        end--;
    }
    return i - (str[0] == '-');
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);


  size_t out_i = 0;
  for (size_t i = 0; fmt[i] != 0; i++) {
    if (fmt[i] == '%') {
      i++;
      switch (fmt[i]) {
        case '%':
          out[out_i] = '%';
          out++;
          break;
        case 's':
          char* str = va_arg(ap, char*);
          strcpy(out + out_i, str);
          out_i += strlen(str);
          break;
        case 'd':
          int value = va_arg(ap, int);
          out_i += intToStr(value, out + out_i);
          break;
        default:
          panic("未知格式化字符");
          break;
      } 
    } else {
      out[out_i] = fmt[i];
      out_i++;
    }
  }
  out[out_i] = 0;
  putstr(out);
  return out_i;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
