// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __adc_h__
#define __adc_h__

#include <stdbool.h>
#include <stdint.h>

// currently, the sampring width is fixed at 11-bits.
void adc_init(void);
void adc_select(uint8_t ch);
bool adc_peek(uint16_t* value);
uint16_t adc_get(uint8_t ch);

#endif  // __rs485_h__