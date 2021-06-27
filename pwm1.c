// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pwm1.h"

#include "io.h"

void pwm1_init() {
  // Use P4.3
  PIN_FUNC |= bPWM1_PIN_X;
  P4_DIR |= (1 << 3);  // Set P4.3 output
  P4_OUT &= ~(1 << 3);  // Set P4.3 low
  P4_PU &= ~(1 << 3);  // Disable P4.3 pull-up

  // Clock divisor
  PWM_CK_SE = 1;  // Fsys(48M) / 1

  // Clear FIFO and count
  PWM_CTRL &= ~bPWM_CLR_ALL;

  // Enable
  PWM_CTRL |= bPWM_OUT_EN;

  // PWM cycle = 2t, duty = 1:1
  PWM_CYCLE = 2;
  PWM_DATA = 1;
}

void pwm1_duty(uint8_t data, uint8_t cycle) {
  PWM_CYCLE = cycle;
  PWM_DATA = data;
}

void pwm1_enable(bool enable) {
  if (enable)
    PWM_CTRL |= bPWM_OUT_EN;
  else
    PWM_CTRL &= ~bPWM_OUT_EN;
}