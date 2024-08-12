#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t i;
  for (i = 0; s[i] != 0; i++) {}
  return i;
}

char *strcpy(char *dst, const char *src) {
  size_t i;
  for (i = 0; src[i] != 0; i++) {
    dst[i] = src[i];
  }
  dst[i] = 0;
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    if (src[i] == 0 || dst[i-1] == 0) {
      dst[i] = 0;
    } else {
      dst[i] = src[i];
    }
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  size_t i;
  for (i = 0; dst[i] != 0; i++) {}
  strcpy(dst+i, src);
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  size_t i;
  for (i = 0; s1[i] == s2[i] && s1[i] != 0; i++) {}
  if (s1[i] == s2[i] && s1[i] == 0){
    return 0;
  } else {
    return s1[i] - s2[i];
  }
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t i;
  for (i = 0; s1[i] == s2[i] && s1[i] != 0 && i < n; i++) {}
  if (s1[i] == s2[i] && s1[i] == 0){
    return 0;
  } else {
    return s1[i] - s2[i];
  }
}

void *memset(void *s, int c, size_t n) {
  uint8_t* p = (uint8_t*)s;
  for (size_t i = 0; i < n; i++) {
    p[i] = (uint8_t)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  uint8_t* dstp = (uint8_t*)dst;
  uint8_t* srcp = (uint8_t*)src;
  if (dstp < srcp) {
    // 从前往后
    for (size_t i = 0; i < n; i++) {
      dstp[i] = srcp[i];
    }
  } else {
    // 从后往前
    for (size_t i = n; i > 0; i--) {
      dstp[i] = srcp[i];
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  uint8_t* dst = (uint8_t*)out;
  uint8_t* src = (uint8_t*)in;
  size_t i;
  for (i = 0; i < n; i++) {
    dst[i] = src[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  uint8_t* p1 = (uint8_t*)s1;
  uint8_t* p2 = (uint8_t*)s2;
  size_t i;
  for (i = 0; p1[i] == p2[i] && i < n; i++) {}
  if (i == n){
    return 0;
  } else {
    return p1[i] - p2[i];
  }
  return 0;
}

#endif
