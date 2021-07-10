// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "rs485.h"

#include "io.h"

void rs485_init() {
  SER1_IER |= bIER_PIN_MOD1;  // Use XA/XB

  SER1_LCR |= bLCR_DLAB;  // Allow SER1_DIV, SER1_DLM, and SER1_DLL use
  SER1_DIV = 1;
  // { SER1_DLM, SER1_DLL } = Fsys(48M) * 2 / SER1_DIV / 16 / baudrate(115200)
  SER1_DLM = 0;
  SER1_DLL = 52;  // should be set before enabling FIFO
  SER1_LCR &= ~bLCR_DLAB;

  // no parity, stop bit 1-bit, no interrupts by default
  SER1_LCR |= bLCR_WORD_SZ0 | bLCR_WORD_SZ1;  // data length 8-bits
  SER1_MCR |= bMCR_HALF;                      // enable half-duplex mode
  SER1_FCR = bFCR_FIFO_EN;                    // Enable FIFO
}

void rs485_send(uint8_t val) {
  while (!(SER1_LSR & bLSR_T_FIFO_EMP))
    ;
  SER1_FIFO = val;
}

bool rs485_ready() {
  return (SER1_LSR & bLSR_DATA_RDY) != 0;
}

uint8_t rs485_recv() {
  while (!rs485_ready())
    ;
  return SER1_FIFO;
}