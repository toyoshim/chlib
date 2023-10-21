// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "uart1.h"

#include "io.h"

void uart1_init(uint8_t options, uint8_t speed) {
  if (options & UART1_RS485) {
    // Enable half-duplex mode.
    SER1_MCR |= bMCR_HALF;
    // Disable ALE clock.
    XBUS_AUX &= ~bALE_CLK_EN;
    // Use only XA/XB pins.
    SER1_IER &= ~bIER_PIN_MOD0;
    SER1_IER |= bIER_PIN_MOD1;
  } else {
    // Disable half-duplex mode.
    SER1_MCR &= ~bMCR_HALF;
    // Enable AL clock to disable iRS485 mode.
    XBUS_AUX |= bALE_CLK_EN;
    if (options & UART1_P4) {
      // RXD1/TXD1 connect P4.0/P4.4.
      SER1_IER |= bIER_PIN_MOD0;
      SER1_IER &= ~bIER_PIN_MOD1;
      P4_DIR &= ~(1 << 0);
      P4_DIR |= (1 << 4);
    } else {
      // RXD1/TXD1 connect P2.6/P2.7.
      SER1_IER &= ~bIER_PIN_MOD0;
      SER1_IER |= bIER_PIN_MOD1;
      P2_DIR &= ~(1 << 6);
      P2_DIR |= (1 << 7);
    }
  }

  // no parity, stop bit 1-bit, no interrupts by default
  SER1_LCR = bLCR_WORD_SZ0 | bLCR_WORD_SZ1;  // data length 8-bits

  uart1_set_speed(speed);
}

void uart1_set_speed(uint8_t speed) {
  SER1_FCR &= ~bFCR_FIFO_EN;  // Disable FIFO
  SER1_LCR |= bLCR_DLAB;      // Allow SER1_DIV, SER1_DLM, and SER1_DLL use
  SER1_DIV = 1;
  switch (speed) {
    case UART1_115200:
      // { SER1_DLM, SER1_DLL } = Fsys(48M) * 2 / SER1_DIV / 16 /
      // baudrate(115200)
      SER1_DLM = 0;
      SER1_DLL = 52;  // should be set before enabling FIFO
      break;
    case UART1_1M:
      // { SER1_DLM, SER1_DLL } = Fsys(48M) * 2 / SER1_DIV / 16 / baudrate(1M)
      SER1_DLM = 0;
      SER1_DLL = 6;  // should be set before enabling FIFO
      break;
    case UART1_3M:
      // { SER1_DLM, SER1_DLL } = Fsys(48M) * 2 / SER1_DIV / 16 / baudrate(3M)
      SER1_DLM = 0;
      SER1_DLL = 2;  // should be set before enabling FIFO
      break;
  }
  SER1_LCR &= ~bLCR_DLAB;
  SER1_FCR = bFCR_FIFO_EN;  // Enable FIFO
}

void uart1_send(uint8_t val) {
  while (!(SER1_LSR & bLSR_T_FIFO_EMP))
    ;
  SER1_FIFO = val;
}

bool uart1_sent(void) {
  return SER1_LSR & bLSR_T_ALL_EMP;
}

bool uart1_ready(void) {
  return (SER1_LSR & bLSR_DATA_RDY) != 0;
}

uint8_t uart1_recv(void) {
  while (!uart1_ready())
    ;
  return SER1_FIFO;
}