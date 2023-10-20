// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __gpio_h__
#define __gpio_h__

#include <stdbool.h>
#include <stdint.h>

#include "interrupt.h"

// NOTE: Port 5 is not fully configurable and is not supported by following
// APIs. Open drain setting is out of scope, and 5mA push-pull is always used.
// See gpio_enable_opendrain() for the open drain configuration.

// mode
#define INPUT 0
#define INPUT_PULLUP 1
#define OUTPUT 2

void pinMode(uint8_t port, uint8_t bit, uint8_t mode);

// value
#define LOW 0
#define HIGH 1

void digitalWrite(uint8_t port, uint8_t bit, uint8_t value);
uint8_t digitalRead(uint8_t port, uint8_t bit);

#define P0_IN P0
#define P1_IN P1
#define P2_IN P2
#define P3_IN P3
// P4_IN is directly defined
#define P0_OUT P0
#define P1_OUT P1
#define P2_OUT P2
#define P3_OUT P3
// P4_OUT is directly defined

void gpio_enable_interrupt(uint8_t ie, bool high_priority);
void gpio_enable_opendrain(uint8_t port);

#endif  // __gpio_h__