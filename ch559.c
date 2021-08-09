// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ch559.h"

#include <stdarg.h>

#include "io.h"
#include "serial.h"

void (*runBootloader)() = 0xf400;

int putchar(int c) {
  while (!TI)
    ;
  TI = 0;
  SBUF = c & 0xff;
  return c;
}

static void delayU8us(uint8_t us) {
  // clang-format off
  us;
__asm
  mov r7,dpl
loop1$:
  mov a,#8
loop2$:
  dec a 
  jnz loop2$
  nop
  nop
  dec r7
  mov a,r7
  jnz loop1$
__endasm;
  // clang-format on
}

static inline void enter_safe_mode() {
  SAFE_MOD = 0x55;
  SAFE_MOD = 0xaa;
}

static inline void leave_safe_mode() {
  SAFE_MOD = 0;
}

void initialize() {
  // Clock
  // Fosc = 12MHz, Fpll = 288MHz, Fusb4x = 48MHz by PLL_CFG default
  enter_safe_mode();
  CLOCK_CFG = (CLOCK_CFG & ~MASK_SYS_CK_DIV) | 6;  // Fsys = 288MHz / 6 = 48MHz
  PLL_CFG =
      ((24 << 0) | (6 << 5)) & 0xff;  // PLL multiplier 24, USB clock divisor 6
  leave_safe_mode();

#ifndef _NO_UART0
  // UART0 115200 TX at P0.3
  P0_DIR |= 0x08;            // Set P0.3(TXD) as output
  P0_PU |= 0x08;             // Pull-up P0.3(TXD)
  PIN_FUNC |= bUART0_PIN_X;  // RXD0/TXD0 enable P0.2/P0.3
#endif

  SM0 = 0;  // 8-bits data
  SM1 = 1;  // variable baud rate, based on timer

  TMOD |= bT1_M1;               // Timer1 mode2
  T2MOD |= bTMR_CLK | bT1_CLK;  // use original Fsys, timer1 faster clock
  PCON |= SMOD;                 // fast mode
  TH1 = 230;                    // 256 - Fsys(48M) / 16 / baudrate(115200)

  TR1 = 1;  // Start timer1
  TI = 1;   // Set transmit interrupt flag for the first transmit

  // GPIO
  PORT_CFG = 0x00;  // 5mA push-pull for port 0-3 by default

  // SerialLibrary
  serial_init();

  if (RESET_KEEP) {
    RESET_KEEP = 0;
    Serial.println("bootloader");
    runBootloader();
  }
  RESET_KEEP = 1;
}

void delayMicroseconds(uint32_t us) {
  while (us > 255) {
    delayU8us(255);
    us -= 255;
  }
  delayU8us(us & 0xff);
}

void delay(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i)
    delayMicroseconds(1000);
}

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
      default:
        Serial.println("N/A");
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
      default:
        Serial.println("N/A");
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
      default:
        Serial.println("N/A");
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
      default:
        Serial.println("N/A");
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
      default:
        Serial.println("N/A");
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
      default:
        Serial.println("N/A");
        break;
    }
  }
}

uint8_t digitalReadPort(uint8_t port) {
  switch (port) {
    case 0:
      return P0;
    case 1:
      return P1;
    case 2:
      return P2;
    case 3:
      return P3;
    case 4:
      return P4_IN;
    default:
      Serial.println("N/A");
      return 0;
  }
}

uint8_t digitalRead(uint8_t port, uint8_t pin) {
  uint8_t v = digitalReadPort(port);
  return (v & (1 << pin)) ? HIGH : LOW;
}