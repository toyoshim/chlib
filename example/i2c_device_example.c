// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "ch559.h"
#include "i2c.h"
#include "serial.h"

bool start(uint8_t address, uint8_t dir) {
  Serial.printf("ADDR: $%x, dir: %d\n", address, dir);
  return address == 0x50;
}

bool write(uint8_t data) {
  Serial.printf("DATA: $%x\n", data);
  return true;
}

bool read(uint8_t* data) {
  Serial.println("READ");
  *data = 0x55;
  return true;
}

void end(void) {
  // Serial.println("END");
}

void main(void) {
  initialize();
  Serial.println("I2C Device Example");

  struct i2c i2c;
  memset(&i2c, 0, sizeof(i2c));
  i2c.sda = I2C_SDA_P0_2;
  i2c.mode = I2C_MODE_DEVICE;
  i2c.start = start;
  i2c.write = write;
  i2c.read = read;
  i2c.end = end;
  i2c_init(&i2c);

  for (;;)
    ;
}