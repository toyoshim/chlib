// Copyright 2024 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "ch559.h"
#include "gpio.h"
#include "serial.h"
#include "timer3.h"

void main(void) {
  initialize();
  Serial.println("Timer3 Example");

  timer3_tick_init();
  pinMode(1, 5, OUTPUT);
  uint8_t value = LOW;
  uint16_t last = timer3_tick_msec();

  for (;;) {
    if (timer3_tick_msec_between(last, last + 500)) {
      continue;
    }
    last += 500;
    digitalWrite(1, 5, value);
    value = value == LOW ? HIGH : LOW;
  }
}