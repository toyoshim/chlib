// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __ch559_h__
#define __ch559_h__

#include <stdbool.h>
#include <stdint.h>

void initialize(void);

// Arduino-like APIs

void delayMicroseconds(uint32_t us);
void delay(uint32_t ms);

#endif  // __ch559_h__
