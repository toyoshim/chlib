// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "ch559.h"
#include "i2c.h"
#include "serial.h"

void main(void) {
  initialize();
  Serial.println("I2C Host Example");

  struct i2c i2c;
  memset(&i2c, 0, sizeof(i2c));
  i2c.sda = I2C_SDA_P0_2;
  i2c.mode = I2C_MODE_HOST;
  i2c_init(&i2c);

  i2c_begin(0x08, I2C_BEGIN_WRITE);
  i2c_write(0xa0);
  i2c_end();

  i2c_begin(0x08, I2C_BEGIN_READ);
  i2c_read();
  i2c_end();

  i2c_begin(0x08, I2C_BEGIN_WRITE);
  i2c_write(0xa0);
  i2c_begin(0x08, I2C_BEGIN_READ);  // Repeated start
  i2c_read();
  i2c_end();

  for (;;)
    ;
}