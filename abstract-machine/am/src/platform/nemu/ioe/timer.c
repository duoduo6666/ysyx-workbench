#include <am.h>
#include <nemu.h>
#include <stdint.h>
#include <stdio.h>

uint64_t rtc_start_time;
void __am_timer_init() {
  uint32_t* ptr = (uint32_t*)&rtc_start_time;
  ptr[0] = inl(RTC_ADDR);
  ptr[1] = inl(RTC_ADDR+4);
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uint64_t time;
  uint32_t* ptr = (uint32_t*)&time;
  ptr[0] = inl(RTC_ADDR);
  ptr[1] = inl(RTC_ADDR+4);
  uptime->us = time - rtc_start_time;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
