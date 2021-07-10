// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __timer3_h__
#define __timer3_h__

#include <stdbool.h>
#include <stdint.h>

#include "interrupt.h"

extern void timer3_int() __interrupt INT_NO_TMR3 __using 1;

void timer3_tick_init();
uint16_t timer3_tick_raw();
uint16_t timer3_tick_from_usec(uint16_t usec);
uint16_t timer3_tick_msec();
uint16_t timer3_tick_sec();
bool timer3_tick_raw_between(uint16_t begin, uint16_t end);   // inclusive
bool timer3_tick_msec_between(uint16_t begin, uint16_t end);  // inclusive

#endif  // __timer3_h__