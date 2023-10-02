// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "adc.h"

#include "ch559.h"
#include "io.h"

void adc_init(void) {
  P1_IE = 0;  // Port 1 disable
  ADC_SETUP = bADC_POWER_EN;
  ADC_CK_SE = 2;  // clock not divided
  ADC_CTRL = 0;
  ADC_CHANN = 1;                // use AIN0
  ADC_EX_SW = bADC_RESOLUTION;  // 11-bits
  delayMicroseconds(100);
}

uint16_t adc_get(uint8_t ch) {
  ADC_CHANN = (1 << ch);
  delayMicroseconds(10);
  ADC_CTRL |= bADC_SAMPLE;
  delayMicroseconds(5);
  ADC_CTRL &= ~bADC_SAMPLE;
  while (!(ADC_STAT & bADC_IF_ACT))  // wait til completion
    ;
  ADC_STAT |= bADC_IF_ACT;  // clear
  return (ADC_FIFO_H << 8) | ADC_FIFO_L;
}