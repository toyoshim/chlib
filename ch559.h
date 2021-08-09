// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __ch559_h__
#define __ch559_h__

#include <stdbool.h>
#include <stdint.h>

void initialize();

// Arduino-like APIs

void delayMicroseconds(uint32_t us);
void delay(uint32_t ms);

// mode
#define INPUT 0
#define INPUT_PULLUP 1
#define OUTPUT 2
// note: port 5 is not supported yet
// open drain setting is out of scope, and 5mA push-pull is always used
void pinMode(uint8_t port, uint8_t bit, uint8_t mode);

// value
#define LOW 0
#define HIGH 1
// note: port 4-5 is not supported yet
void digitalWrite(uint8_t port, uint8_t bit, uint8_t value);
// note: port 5 is not supported yet
uint8_t digitalReadPort(uint8_t port);
uint8_t digitalRead(uint8_t port, uint8_t bit);

#endif  // __ch559_h__
