// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpio.h"

#include "io.h"
#include "serial.h"

void pinMode(uint8_t port, uint8_t bit, uint8_t mode) {
  uint8_t mask = 1 << bit;
  if (mode == INPUT_PULLUP) {
    switch (port) {
      case 0:
        P0_PU |= mask;
        break;
      case 1:
        P1_PU |= mask;
        break;
      case 2:
        P2_PU |= mask;
        break;
      case 3:
        P3_PU |= mask;
        break;
      case 4:
        P4_PU |= mask;
        break;
    }
  } else {
    mask = ~mask;
    switch (port) {
      case 0:
        P0_PU &= mask;
        break;
      case 1:
        P1_PU &= mask;
        break;
      case 2:
        P2_PU &= mask;
        break;
      case 3:
        P3_PU &= mask;
        break;
      case 4:
        P4_PU &= mask;
        break;
    }
    mask = ~mask;
  }
  if (mode == OUTPUT) {
    switch (port) {
      case 0:
        P0_DIR |= mask;
        break;
      case 1:
        P1_DIR |= mask;
        break;
      case 2:
        P2_DIR |= mask;
        break;
      case 3:
        P3_DIR |= mask;
        break;
      case 4:
        P4_DIR |= mask;
        break;
    }
  } else {
    mask = ~mask;
    switch (port) {
      case 0:
        P0_DIR &= mask;
        break;
      case 1:
        P1_DIR &= mask;
        break;
      case 2:
        P2_DIR &= mask;
        break;
      case 3:
        P3_DIR &= mask;
        break;
      case 4:
        P4_DIR &= mask;
        break;
    }
  }
}

void digitalWrite(uint8_t port, uint8_t bit, uint8_t value) {
  uint8_t mask = 1 << bit;
  if (value == HIGH) {
    switch (port) {
      case 0:
        P0 |= mask;
        break;
      case 1:
        P1 |= mask;
        break;
      case 2:
        P2 |= mask;
        break;
      case 3:
        P3 |= mask;
        break;
      case 4:
        P4_OUT |= mask;
        break;
    }
  } else {
    mask = ~mask;
    switch (port) {
      case 0:
        P0 &= mask;
        break;
      case 1:
        P1 &= mask;
        break;
      case 2:
        P2 &= mask;
        break;
      case 3:
        P3 &= mask;
        break;
      case 4:
        P4_OUT &= mask;
        break;
    }
  }
}

uint8_t digitalRead(uint8_t port, uint8_t pin) {
  uint8_t v;
  switch (port) {
    case 0:
      v = P0;
      break;
    case 1:
      v = P1;
      break;
    case 2:
      v = P2;
      break;
    case 3:
      v = P3;
      break;
    case 4:
      v = P4_IN;
      break;
    default:
      v = 0;
      break;
  }
  return (v & (1 << pin)) ? HIGH : LOW;
}

void gpio_enable_interrupt(uint8_t ie, bool high_priority) {
  GPIO_IE = ie;
  IE_GPIO = 1;  // Enable GPIO interrupt
  EA = 1;       // Enable interrupt
  if (high_priority) {
    IP_EX |= bIP_GPIO;
  } else {
    IP_EX &= ~bIP_GPIO;
  }
}

void gpio_enable_opendrain(uint8_t port) {
  PORT_CFG |= (1 << port);
}