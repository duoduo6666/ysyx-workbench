#include <am.h>
#include <nemu.h>
#include <stdio.h>
#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t value = inl(KBD_ADDR);
  kbd->keydown = value & KEYDOWN_MASK;
  kbd->keycode = value & 0xff;
}
