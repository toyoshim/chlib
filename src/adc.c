// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "adc.h"

#include "ch559.h"
#include "io.h"

void adc_init(void) {
  ADC_SETUP = bADC_POWER_EN;    // ADC power enabled.
  ADC_CK_SE = 4;                // Set 4 for frequency division.
  ADC_CTRL = 0;                 // Manual sampling.
  ADC_EX_SW = bADC_RESOLUTION;  // 11-bits
}

void adc_select(uint8_t ch) {
  uint8_t mask = (1 << ch);
  P1_IE &= ~mask;
  ADC_CHANN = mask;

  // Take the first sample.
  ADC_CTRL |= bADC_SAMPLE;
  ADC_CTRL &= ~bADC_SAMPLE;
}

bool adc_peek(uint16_t* value) {
  if (!(ADC_STAT & bADC_IF_ACT))
    return false;

  // Write 1 to reset.
  ADC_STAT |= bADC_IF_ACT;

  // Read the value.
  *value = (ADC_FIFO_H << 8) | ADC_FIFO_L;

  // Take the next sample.
  ADC_CTRL |= bADC_SAMPLE;
  ADC_CTRL &= ~bADC_SAMPLE;

  return true;
}

uint16_t adc_get(uint8_t ch) {
  adc_select(ch);
  uint16_t value;
  while (!adc_peek(&value))
    ;
  return value;
}