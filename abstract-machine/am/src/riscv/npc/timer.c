#include <am.h>

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uint32_t lo = *(volatile uint32_t *)0x20000000;
  uint32_t hi = *(volatile uint32_t *)0x20000004;
  uptime->us = ((uint64_t)hi << 32) | lo;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->year   = *(volatile uint32_t *)0x20000010;
  rtc->month  = *(volatile uint32_t *)0x20000014;
  rtc->day    = *(volatile uint32_t *)0x20000018;
  rtc->hour   = *(volatile uint32_t *)0x2000001c;
  rtc->minute = *(volatile uint32_t *)0x20000020;
  rtc->second = *(volatile uint32_t *)0x20000024;
}
