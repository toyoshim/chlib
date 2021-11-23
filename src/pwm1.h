// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __pwm1_h__
#define __pwm1_h__

#include <stdbool.h>
#include <stdint.h>

// note: pin is fixed to P4.3 for now
void pwm1_init();
void pwm1_enable(bool enable);
void pwm1_duty(uint8_t data, uint8_t cycle);

#endif  // __pwm1_h__
