// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __i2c_h__
#define __i2c_h__

#include <stdbool.h>
#include <stdint.h>

#include "interrupt.h"
#include "timer3.h"

extern void i2c_int_gpio(void) __interrupt(INT_NO_GPIO) __using(0);
extern void i2c_int_tmr0(void) __interrupt(INT_NO_TMR0) __using(0);

enum {
  I2C_INVALID = 0,
  I2C_SDA_P0_2 = 1,
  // TODO: support more choices

  I2C_MODE_DEVICE = 0,
  I2C_MODE_HOST = 1,

  I2C_BEGIN_WRITE = 0,
  I2C_BEGIN_READ = 1,

  I2C_DIR_WRITE = 0,
  I2C_DIR_READ = 1,
};

struct i2c {
  // Other interrupt ports that should be enabled and handled b the user
  // interrupt handler.
  uint8_t ie;
  // Only specific GPIO can be used for interrupt supports.
  uint8_t sda;
  // Operation mode.
  uint8_t mode;

  // Set non-zero value if you don't want to run the `interrupt_handler` in a
  // certain timespan after I2C accesses.
  uint16_t exclusive_time_raw;
  void (*interrupt_handler)(void);

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
bool i2c_set_mode(uint8_t mode);
void i2c_update_interrupt(uint8_t ie);
bool i2c_begin(uint8_t address, uint8_t rw);
bool i2c_write(uint8_t data);
uint8_t i2c_read(void);
void i2c_end(void);

#endif  // __i2c_h__