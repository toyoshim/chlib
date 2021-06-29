// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __led_h__
#define __led_h__

#include <stdint.h>

// Should be included from the main source file to set up interrupt handler.
#include "timer3.h"

// LED mode
enum {
  L_ON,
  L_BLINK,
  L_FAST_BLINK,
  L_FASTER_BLINK,
  L_BLINK_TWICE,
  L_BLINK_THREE_TIMES,
  L_OFF,
};

void led_init(uint8_t port, uint8_t pin, uint8_t polarity);
void led_mode(uint8_t mode);
void led_oneshot(uint8_t shot);
void led_poll();

#endif  // __led_h__