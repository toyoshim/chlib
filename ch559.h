// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __ch559_h__
#define __ch559_h__

#include <stdbool.h>
#include <stdint.h>

#include "interrupt.h"

extern void timer3_int() __interrupt INT_NO_TMR3 __using 1;

void initialize();

// note: pins are fixed to XA/XB for now
void rs485_init();
void rs485_send(uint8_t val);
bool rs485_ready();
uint8_t rs485_recv();

// note: pin is fixed to P4.3 for now
void pwm1_init();
void pwm1_enable(bool enable);
void pwm1_duty(uint8_t data, uint8_t cycle);

void timer3_tick_init();
uint16_t timer3_tick_raw();
uint16_t timer3_tick_from_usec(uint16_t usec);
uint16_t timer3_tick_msec();
uint16_t timer3_tick_sec();
bool timer3_tick_raw_between(uint16_t begin, uint16_t end);  // inclusive
bool timer3_tick_msec_between(uint16_t begin, uint16_t end);  // inclusive

// Arduino-like APIs

// type
#define BIN 0  // support uint8
#define HEX 1  // support uint8
#define DEC 2  // support int16
extern struct SerialLibrary {
  void (*putc)(uint8_t val);
  void (*printc)(int16_t val, uint8_t type);
  void (*print)(const char* val);
  void (*println)(const char* val);
  void (*printf)(const char*fmt, ...);
} Serial;

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

#endif  // __ch559_h__
