#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

enum target {
  TO_STDOUT = 0,
  TO_STRING = 1,
};

size_t format_write_char(bool target, char* string, char ch) {
  if (target == TO_STDOUT) {
    putch(ch);
    return 1;
  } else if (target == TO_STRING) {
    *string = ch;
    return 1;
  } else {
    assert(0);
  }
  assert(0);
}

size_t format_write_int(bool target, char* string, int value) {
  char buf[13];
  size_t i = 0;

  // 负数
  if (value < 0) {
      buf[i] = '-';
      i++;
      value = -value;
  }

  for (; value != 0; i++) {
      buf[i] = (value % 10) + '0';
      value /= 10;
  }

  for (int t = i; t > 0; t--) {
    format_write_char(target, string+i-t, buf[t-1]);
  }
  // int start = str[0] == '-';
  // int end = i - 1;
  // while (start < end) {
  //     char t = str[start];
  //     str[start] = str[end];
  //     str[end] = t;
  //     start++;
  //     end--;
  // }
  return i;

}

int format(bool target, const char *fmt, char* out, va_list ap) {
  putstr("format\n");
  int out_i = 0;
  for (size_t i = 0; fmt[i] != 0; i++) {
    if (fmt[i] == '%') {
      i++;
      switch (fmt[i]) {
        case '%':
          out_i += format_write_char(target, &(out[out_i]), '%');
          break;
        case 'c':
          char ch = va_arg(ap, int);
          out_i += format_write_char(target, &(out[out_i]), ch);
          break;
        case 's':
          char* str = va_arg(ap, char*);
          for (size_t t = 0; t < strlen(str); t++) {
            out_i += format_write_char(target, &(out[out_i]), str[t]);
          }
          break;
        case 'd':
          int value = va_arg(ap, int);
          out_i += format_write_int(target, out + out_i, value);
          break;
        default:
          panic("未知格式化字符");
          break;
      } 
    } else {
      format_write_char(target, &(out[out_i]), fmt[i]);
      out_i++;
    }
  }
  return out_i;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int len = format(TO_STDOUT, fmt, 0, ap);
  return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int len = format(TO_STRING, fmt, out, ap);
  out[len] = 0;
  return len+1;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
