// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __serial_h__
#define __serial_h__

#include <stdint.h>

// type
#define BIN 0  // support uint8
#define HEX 1  // support uint8
#define DEC 2  // support int16
extern struct SerialLibrary {
  void (*putc)(uint8_t val);
  void (*printc)(int16_t val, uint8_t type);
  void (*print)(const char* val);
  void (*println)(const char* val);
  void (*printf)(const char* fmt, ...);
} Serial;

void serial_init(void);

#endif  // __serial_h__
