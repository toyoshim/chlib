// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "timer3.h"

#include "io.h"

static uint16_t timer3_tick = 0;

void timer3_int() __interrupt INT_NO_TMR3 __using 1 {
  timer3_tick++;
  T3_STAT |= bT3_IF_END;
}

static bool timer3_tick_ge(uint16_t tick) {
  uint8_t h = tick >> 8;
  uint8_t t = T3_COUNT_H;
  if (t < h)
    return false;
  if (t > h)
    return true;
  uint8_t l = tick;
  t = T3_COUNT_L;
  if (t >= l)
    return true;
  return T3_COUNT_H > h;
}

static bool timer3_tick_le(uint16_t tick) {
  uint8_t h = tick >> 8;
  uint8_t t = T3_COUNT_H;
  if (t > h)
    return false;
  if (t < h)
    return true;
  uint8_t l = tick;
  t = T3_COUNT_L;
  if (t > l)
    return false;
  return T3_COUNT_H <= h;
}

void timer3_tick_init() {
  T3_SETUP |= bT3_EN_CK_SE;  // Enable to access divisor settings register
  T3_CK_SE_L = 0x80;         // Clock = Fsys(48M) / 48k = 1kHz
  T3_CK_SE_H = 0xbb;
  T3_SETUP &= ~bT3_EN_CK_SE;  // Disable
  T3_END_L = 0x70;
  T3_END_H = 0x3e;  // 1000 * 16
  T3_CTRL |= bT3_CLR_ALL;
  T3_CTRL &= ~bT3_CLR_ALL;
  T3_SETUP |= bT3_IE_END;  // Enable end interrupt
  T3_CTRL |= bT3_CNT_EN;   // Start counting
  T3_STAT = 0xff;
  IE_TMR3 = 1;  // Enable timer3 interrupt
  EA = 1;       // Enable interruprts
}

uint16_t timer3_tick_raw() {
  uint16_t tick;
  for (;;) {
    uint8_t h = T3_COUNT_H;
    uint8_t l = T3_COUNT_L;
    if (h == T3_COUNT_H) {
      tick = (h << 8) | l;
      break;
    }
  }
  return tick;
}

uint16_t timer3_tick_from_usec(uint16_t usec) {
  uint16_t x16 = usec * 16;
  uint16_t tick = x16 / 1000;
  if (x16 % 1000)
    tick++;
  return tick;
}

uint16_t timer3_tick_msec() {
  return timer3_tick_raw() >> 4;
}

uint16_t timer3_tick_sec() {
  return timer3_tick;
}

bool timer3_tick_raw_between(uint16_t begin, uint16_t end) {
  if (begin < end)
    return timer3_tick_ge(begin) && timer3_tick_le(end);
  return timer3_tick_ge(begin) || timer3_tick_le(end);
}

bool timer3_tick_msec_between(uint16_t begin, uint16_t end) {
  return timer3_tick_raw_between(begin << 4, end << 4);
}
