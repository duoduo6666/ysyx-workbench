#include <am.h>
#include <nemu.h>
#include <string.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)

void __am_audio_init() {
  outl(AUDIO_INIT_ADDR, 1);
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = inl(AUDIO_SBUF_SIZE_ADDR);
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
  __am_audio_init();
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  uint32_t len = ctl->buf.end - ctl->buf.start;
  uint32_t sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);

  while (len > sbuf_size - inl(AUDIO_COUNT_ADDR)) {}
  
  outl(AUDIO_INIT_ADDR, 2);
  memcpy((uint8_t*)AUDIO_SBUF_ADDR + inl(AUDIO_COUNT_ADDR), ctl->buf.start, len);
  outl(AUDIO_COUNT_ADDR, inl(AUDIO_COUNT_ADDR)+len);
  outl(AUDIO_INIT_ADDR, 3);
  
}
