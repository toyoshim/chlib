// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __adc_h__
#define __adc_h__

#include <stdint.h>

void adc_init(void);
uint16_t adc_get(uint8_t ch);  // 11-bit

#endif  // __rs485_h__