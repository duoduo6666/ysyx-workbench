#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

enum target {
  TO_STDOUT = 0,
  TO_STRING = 1,
};

bool format_padding;
int format_width;

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
      format_write_char(target, string, '-');
      string++;
      value = -value;
  }

  for (; value != 0 || i == 0; i++) {
      buf[i] = (value % 10) + '0';
      value /= 10;
  }

  if (format_width > 0) {
    for (int t = i; t < format_width; t++) {
      if (format_padding) {
        format_write_char(target, string+i-t, '0');
      } else {
        format_write_char(target, string+i-t, ' ');
      }
      string++;
    }

  }
  
  for (int t = i; t > 0; t--) {
    format_write_char(target, string+i-t, buf[t-1]);
  }
  return i;
}

size_t format_write_hex(bool target, char* string, unsigned int value) {
  char buf[8];
  size_t i = 0;

  for (; value != 0 || i == 0; i++) {
      buf[i] = (value & 0b1111) + '0';
      if (buf[i] > '9') buf[i] += 'a' - '9' - 1;
      value >>= 4;
  }

  if (format_width > 0) {
    for (int t = i; t < format_width; t++) {
      if (format_padding) {
        format_write_char(target, string+i-t, '0');
      } else {
        format_write_char(target, string+i-t, ' ');
      }
      string++;
    }

  }
  
  for (int t = i; t > 0; t--) {
    format_write_char(target, string+i-t, buf[t-1]);
  }
  return i;
}

int str2int(const char *str, size_t *index) {
  int value = 0;
  int t = 1;
  int i;
  for (i = 0; str[i] >= '0' && str[i] <= '9'; i++) {
    t *= 10;
    *index += 1;
  }
  *index -= 1;

  for (i = 0; t > 0; i++) {
    t /= 10;
    value += (str[i]-'0') * (t);
  }
  return value;
}


int format(bool target, const char *fmt, char* out, va_list ap) {
  format_padding = 0;
  format_width = 0;
  int out_i = 0;
  for (size_t i = 0; fmt[i] != 0; i++) {
    if (fmt[i] == '%') {
      bool end = 0;
      while (end == 0) {
        i++;
        switch (fmt[i]) {
          case '0':
            format_padding = 1;
            break;
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            format_width = str2int(&(fmt[i]), &i);
            break;
          case '%':
            out_i += format_write_char(target, &(out[out_i]), '%');
            end = 1;
            break;
          case 'c':
            char ch = va_arg(ap, int);
            out_i += format_write_char(target, &(out[out_i]), ch);
            end = 1;
            break;
          case 's':
            char* str = va_arg(ap, char*);
            for (size_t t = 0; t < strlen(str); t++) {
              out_i += format_write_char(target, &(out[out_i]), str[t]);
            }
            end = 1;
            break;
          case 'd':
            int value = va_arg(ap, int);
            out_i += format_write_int(target, out + out_i, value);
            format_padding = 0;
            format_width = 0;
            end = 1;
            break;
          case 'l':
            assert(sizeof(long) == sizeof(int));
            break;
          case 'x':
            value = va_arg(ap, int);
            out_i += format_write_hex(target, out + out_i, value);
            format_padding = 0;
            format_width = 0;
            end = 1;
            break;
          case 'p':
            value = (uint32_t)va_arg(ap, void*);
            out_i += format_write_hex(target, out + out_i, value);
            format_padding = 0;
            format_width = 0;
            end = 1;
            break;
          default:
            printf("未知格式化字符 %c\n", fmt[i]);
            assert(0);
            end = 1;
            break;
        } 
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
