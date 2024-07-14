// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "timer3.h"

#include "io.h"

static uint16_t sec_from_init = 0;

// Interrupt handler called per 1 sec to count tick in secounds.
void timer3_int(void) __interrupt(INT_NO_TMR3) __using(1) {
  sec_from_init++;
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

void timer3_tick_init(void) {
  // Setup timer3 clock to 16kHz so that T3_COUNT counts automatically at 16kHz
  T3_SETUP |= bT3_EN_CK_SE;  // Enable to access divisor settings register
  T3_CK_SE_L = 0xb8;         // Clock = Fsys(48M) / 3k(0xbb8) = 16kHz
  T3_CK_SE_H = 0x0b;         // Note: T3_CK_SE is 12bit register

  // Setup timer3 interrupt per 1 sec (16kHz / 0x3e80)
  T3_SETUP &= ~bT3_EN_CK_SE;  // Disable
  T3_END_L = 0x80;
  T3_END_H = 0x3e;  // 1000 * 16
  T3_CTRL |= bT3_CLR_ALL;
  T3_CTRL &= ~bT3_CLR_ALL;
  T3_SETUP |= bT3_IE_END;  // Enable end interrupt
  T3_CTRL |= bT3_CNT_EN;   // Start counting
  T3_STAT = 0xff;
  IE_TMR3 = 1;  // Enable timer3 interrupt
  EA = 1;       // Enable interruprts
}

uint16_t timer3_tick_raw(void) {
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
  return (usec >> 6) + 1;
}

uint16_t timer3_tick_msec(void) {
  return timer3_tick_raw() >> 4;
}

uint16_t timer3_tick_sec(void) {
  return sec_from_init;
}

bool timer3_tick_raw_between(uint16_t begin, uint16_t end) {
  if (begin < end) {
    return timer3_tick_ge(begin) && timer3_tick_le(end);
  }
  return timer3_tick_ge(begin) || timer3_tick_le(end);
}

bool timer3_tick_msec_between(uint16_t begin, uint16_t end) {
  return timer3_tick_raw_between(begin << 4, end << 4);
}
