// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "i2c.h"
#include <stddef.h>

#include "gpio.h"
#include "io.h"
#include "serial.h"

static struct i2c i2c;

static void wait_posedge() {
  while (SCL_BIT)
    ;
  while (!SCL_BIT)
    ;
}

static void wait_negedge() {
  while (!SCL_BIT)
    ;
  while (SCL_BIT)
    ;
}

void i2c_int(void) __interrupt(INT_NO_GPIO) __using(0) {
  uint8_t sda = SDA_BIT;
  if (i2c.interrupt_handler && i2c.interrupt_handler()) {
    return;
  }
  if (sda) {
    return;
  }

  uint8_t addr = 0;
  for (uint8_t bit = 0; bit < 7; ++bit) {
    addr <<= 1;
    wait_posedge();
    addr |= SDA_BIT;
  }
  wait_posedge();
  uint8_t dir = SDA_BIT ? I2C_DIR_READ : I2C_DIR_WRITE;
  wait_negedge();

  // Clock stretch
  SCL_DIR |= SCL_MASK;  // output

  // Address check
  bool ready = i2c.start(addr, dir);
  uint8_t data = 0;
  if (ready && dir == I2C_DIR_READ) {
    ready = i2c.read(&data);
  }
  if (ready) {
    // ACK
    SDA_BIT = LOW;
    SDA_DIR |= SDA_MASK;  // output
  }

  // Release clock streach
  SCL_DIR &= ~SCL_MASK;  // input
  wait_negedge();

  if (ready) {
    // Release ACK
    SDA_DIR &= ~SDA_MASK;  // input
  }

  for (;;) {
    if (dir == I2C_DIR_READ) {
      if (ready) {
        SDA_DIR |= SDA_MASK;  // output
      }
      for (uint8_t bit = 0; bit < 8; ++bit) {
        SDA_BIT = (data & 0x80) ? HIGH : LOW;
        data <<= 1;
        wait_negedge();
      }
      SDA_DIR &= ~SDA_MASK;  // input
      wait_posedge();
      if (SDA_BIT) {
        // MACK
        if (ready) {
          i2c.end();
        }
        // Wait for STOP condition
        wait_posedge();
        return;
      } else {
        // ACK
        wait_negedge();
        if (ready) {
          // Clock stretch
          SCL_DIR |= SCL_MASK;  // output
          if (!i2c.read(&data)) {
            data = 0;
          }
          SDA_BIT = (data & 0x80) ? HIGH : LOW;
          SDA_DIR |= SDA_MASK;  // output
          // Release clock streach
          SCL_DIR &= ~SCL_MASK;  // input
        }
      }
    } else {
      // Check STOP condition
      wait_posedge();
      sda = SDA_BIT;
      data = 0;
      if (sda) {
        data = 1;
      } else {
        while (SCL_BIT) {
          if (SDA_BIT) {
            if (ready) {
              i2c.end();
            }
            return;
          }
        }
      }
      for (uint8_t bit = 1; bit < 8; ++bit) {
        data <<= 1;
        wait_posedge();
        data |= SDA_BIT;
      }
      wait_negedge();
      if (ready) {
        // Clock stretching
        SCL_DIR |= SCL_MASK;  // output

        // ACK(LOW) or NACK(HIGH)
        SDA_BIT = i2c.write(data) ? LOW : HIGH;
        SDA_DIR |= SDA_MASK;  // output

        // Release clock streaching
        SCL_DIR &= ~SCL_MASK;  // input
        wait_negedge();
        // Release ACK or NACK
        SDA_DIR &= ~SDA_MASK;  // input
      } else {
        wait_negedge();
      }
    }  // else
  }    // for
}

bool i2c_init(const struct i2c* opt) {
  if (!opt->start || !opt->write || !opt->read || !opt->end) {
    return false;
  }
  i2c = *opt;
  i2c.ie |= bIE_IO_EDGE;
  if (i2c.sda == I2C_SDA_P0_2) {
    i2c.ie |= bIE_RXD0_LO;
    if (PIN_FUNC & bUART0_PIN_X) {
      REN = 0;  // Disable UART0 receiving as RXD0 was assigned to P0_2.
    }
    pinMode(0, 2, INPUT_PULLUP);
  } else {
    return false;
  }
  SDA_PU |= SDA_MASK;    // pullup
  SDA_DIR &= ~SDA_MASK;  // input
  SCL_PU |= SCL_MASK;    // pullup
  SCL_DIR &= ~SCL_MASK;  // input
  SCL_BIT = LOW;
  gpio_enable_interrupt(i2c.ie, true);
  return true;
}