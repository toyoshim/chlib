// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __led_h__
#define __led_h__

#include <stdint.h>

void led_init(uint8_t port, uint8_t pin, uint8_t polarity);
void led_mode(uint8_t mode);
void led_oneshot(uint8_t shot);
void led_update();

#endif  // __led_h__