// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __i2c_h__
#define __i2c_h__

#include <stdbool.h>
#include <stdint.h>

#include "interrupt.h"

extern void i2c_int(void) __interrupt(INT_NO_GPIO) __using(0);

enum {
  I2C_INVALID = 0,
  I2C_SDA_P0_2 = 1,
  // TODO: support more choices

  I2C_DIR_WRITE = 0,
  I2C_DIR_READ = 1,
};

struct i2c {
  uint8_t ie;   // Other interrupt ports that should be enabled and handled by
                // the user interrupt handler.
  uint8_t sda;  // Only specific GPIO can be used for interrupt supports.
  bool (*interrupt_handler)(void);
  // `start`, `write`, and `read` are called with clock stretch cycle, and can
  // spent a certain time. But `end` cannot.
  bool (*start)(uint8_t address, uint8_t dir);
  bool (*write)(uint8_t data);
  bool (*read)(uint8_t* data);
  void (*end)(void);
};

// Following macro should be set.
// Ex)
// #define SDA_BIT P0_2  // should be aligned with i2c.sda.
// #define SDA_DIR P0_DIR
// #define SDA_PU P0_PU
// #define SDA_MASK (1 << 2)
#if !defined(SDA_BIT) || !defined(SDA_DIR) || !defined(SDA_PU) || \
    !defined(SDA_MASK)
#error
#endif
#if !defined(SCL_BIT) || !defined(SCL_DIR) || !defined(SCL_PU) || \
    !defined(SCL_MASK)
#error
#endif

// Internally enables GPIO edge interrupt. You need to be careful to override
// the setting.
bool i2c_init(const struct i2c* opt);

#endif  // __i2c_h__