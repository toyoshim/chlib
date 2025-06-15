// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "i2c.h"
#include <stddef.h>

#include "gpio.h"
#include "io.h"
#include "serial.h"

enum {
  HOST_BUFFER_SIZE = 16,
  HOST_BUFFER_MASK = HOST_BUFFER_SIZE - 1,

  ACK_ACK = 0,
  ACK_NACK = 1,
  ACK_READY = 0x80,
  ACK_NOT_READY = 0x00,
};

static struct i2c i2c;
static uint8_t ie;
static uint16_t exclusive_time_raw = 0;
static uint16_t last_access_time;
static volatile uint8_t host_state = 0;
static volatile uint8_t host_data = 0;
static volatile uint8_t host_ack = ACK_NOT_READY;

static void wait_posedge(void) {
  while (SCL_BIT)
    ;
  while (!SCL_BIT)
    ;
}

static void wait_negedge(void) {
  while (!SCL_BIT)
    ;
  while (SCL_BIT)
    ;
}

void i2c_int_gpio(void) __interrupt(INT_NO_GPIO) __using(0) {
  if (SDA_BIT) {
    if (i2c.interrupt_handler &&
        (exclusive_time_raw == 0 ||
         !timer3_tick_raw_between(last_access_time,
                                  last_access_time + exclusive_time_raw))) {
      i2c.interrupt_handler();
      exclusive_time_raw = 0;
    }
    return;
  }
  uint8_t addr = 0;
  uint8_t bit;
  for (bit = 0; bit < 7; ++bit) {
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

    exclusive_time_raw = i2c.exclusive_time_raw;
    last_access_time = timer3_tick_raw();
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
      for (bit = 0; bit < 8; ++bit) {
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
        while (!SDA_BIT)
          ;
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
      uint8_t sda = SDA_BIT;
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
      for (bit = 1; bit < 8; ++bit) {
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

void i2c_int_tmr0(void) __interrupt(INT_NO_TMR0) __using(0) {
  switch (host_state & 0x0f) {
    case 0:                 // START Condition
      SDA_DIR |= SDA_MASK;  // output, SDA: Low
      break;
    case 1:                 // START Condition
      SCL_DIR |= SCL_MASK;  // output, SCL: Low
      break;
    case 2:
      if (host_state & 0x80) {
        // DATA n, SDA: wait
      } else {
        // DATA n, SDA: update
        if (host_data & 0x80) {
          SDA_DIR &= ~SDA_MASK;  // input, SDA: High
        } else {
          SDA_DIR |= SDA_MASK;  // output, SDA: Low
        }
        host_data <<= 1;
      }
      break;
    case 3:                  // DATA n, SCL: posedge
      SCL_DIR &= ~SCL_MASK;  // input, SCL: High
      break;
    case 4:  // DATA n, clock stretch check
      if (!SCL_BIT) {
        return;
      }
      if (host_state & 0x80) {
        // DATA n, SDA: read
        host_data <<= 1;
        host_data |= SDA_BIT;
      }
      break;
    case 5:                  // DATA n, SCL: negedge
      SDA_DIR &= ~SDA_MASK;  // input, SDA: release
      SCL_DIR |= SCL_MASK;   // output, SCL: Low
      if ((host_state & 0x7f) != 0x75) {
        host_state += 12;
      }
      break;
    case 6:
      if (host_state & 0x80) {
        // ACK, SDA: ACK
        SDA_DIR |= SDA_MASK;  // output, SDA: Low
      } else {
        // ACK, wait
      }
      break;
    case 7:                  // ACK, SCL posedge
      SCL_DIR &= ~SCL_MASK;  // input, SCL: High
      break;
    case 8:  // ACK, clock stretch check
      if (!SCL_BIT) {
        return;
      }
      if (!(host_state & 0x80)) {
        // SDA: read
        host_ack = SDA_BIT;
      }
      break;
    case 9:                 // ACK, SCL negedge
      SCL_DIR |= SCL_MASK;  // output, SCL: Low
      break;
    case 10:
      host_ack |= ACK_READY;
      if (host_state & 0x80) {
        SDA_DIR &= ~SDA_MASK;  // input, SDA: High
      }
      return;
    case 11:                 // STOP Condition
      SCL_DIR &= ~SCL_MASK;  // input, SCL: High
      break;
    case 12:                 // STOP Condition
      if (!SCL_BIT) {
        return;
      }
      SDA_DIR &= ~SDA_MASK;  // input, SDA: High
      break;
    case 13:
      return;
    case 14:                 // Repeated start condition
      SDA_DIR &= ~SDA_MASK;  // input, SDA: High
      break;
    case 15:
      SCL_DIR &= ~SCL_MASK;  // input, SCL: High
      host_ack |= ACK_READY;
      return;
  }
  host_state++;
}

bool i2c_init(const struct i2c* opt) {
  i2c = *opt;
  ie = bIE_IO_EDGE;

  // Port specific initializations.
  if (i2c.sda == I2C_SDA_P0_2 && PIN_FUNC & bUART0_PIN_X) {
    // P0_2 can be used for an interrupt trigger only when the port is
    // specified as UART0 RXD.
    ie |= bIE_RXD0_LO;
    // Disable UART0 receiving, but still it can not be used as a GPIO.
    // Another port also needs to be assigned to drive the SDA line.
    REN = 0;
  } else {
    // Not supported.
    return false;
  }

  // Mode specific initializations.
  return i2c_set_mode(i2c.mode);
}

bool i2c_set_mode(uint8_t mode) {
  i2c.mode = mode;

  if (mode == I2C_MODE_DEVICE) {
    // Disable host mode feature, timer0 interrupt
    ET0 = 0;

    if (!i2c.start || !i2c.write || !i2c.read || !i2c.end) {
      return false;
    }

    SCL_BIT = LOW;

    last_access_time = timer3_tick_raw();
  } else {
    SDA_BIT = LOW;
    SCL_BIT = LOW;

    host_state = 0;

    // Timer0 mode 2, 8-bit auto
    // 100kHz x4 = 48M / (256 - TH0:136)
    TMOD = (TMOD | bT0_M1) & ~bT0_M0;
    T2MOD |= bT0_CLK;  // faster clock
    TL0 = 0;
    TH0 = 136;  // auto load value
    ET0 = 1;
  }
  SDA_DIR &= ~SDA_MASK;  // input
  SDA_PU |= SDA_MASK;    // pullup
  SCL_DIR &= ~SCL_MASK;  // input
  SCL_PU |= SCL_MASK;    // pullup

  i2c_update_interrupt(i2c.ie);

  return true;
}

void i2c_update_interrupt(uint8_t new_ie) {
  i2c.ie = new_ie;
  if (i2c.mode == I2C_MODE_DEVICE) {
    new_ie |= ie;
  }
  gpio_enable_interrupt(new_ie, true);
}

bool i2c_begin(uint8_t address, uint8_t rw) {
  if (host_state) {
    // Repeated start condition
    host_state = 14;
    host_ack = ACK_NOT_READY;  // Reset ack status
    TR0 = 1;                   // Timer start
    while (!(host_ack & ACK_READY))
      ;
    TR0 = 0;  // Timer stop
  }
  return i2c_write((address << 1) | rw);
}

bool i2c_write(uint8_t data) {
  host_data = data;
  host_state = 0;            // Start with START condition
  host_ack = ACK_NOT_READY;  // Reset ack status
  TR0 = 1;                   // Timer start

  while (!(host_ack & ACK_READY))
    ;
  TR0 = 0;  // Timer stop
  return !(host_ack & ACK_NACK);
}

uint8_t i2c_read(void) {
  host_data = 0;
  host_state = 0x82;  // Start with DATA read
  host_ack = ACK_NOT_READY;
  TR0 = 1;  // Timer start

  while (!(host_ack & ACK_READY))
    ;
  TR0 = 0;  // Timer stop
  return host_data;
}

void i2c_end(void) {
  TR0 = 1;  // Timer start
  host_state = 11;
  while (host_state != 13)
    ;
  TR0 = 0;  // Timer stop
  host_state = 0;
}