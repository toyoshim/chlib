// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __uart1_h__
#define __uart1_h__

#include <stdbool.h>
#include <stdint.h>

enum {
  // for uart1_init
  UART1_RS485 = (1 << 0),  // iRS4985 with XA/XB
  UART1_P2 = (0 << 1),     // UART with P2.6/P2.7
  UART1_P4 = (1 << 1),     // UART with P4.0/P4.4

  // for uart1_set_speed
  UART1_115200 = 0,  // 115200bps
  UART1_1M = 1,      // 1Mbps
  UART1_3M = 2,      // 3Mbps
};

void uart1_init(uint8_t options, uint8_t speed);
void uart1_set_speed(uint8_t speed);
inline void uart1_send(uint8_t val);
inline bool uart1_sent();
inline bool uart1_ready();
inline uint8_t uart1_recv();

#endif  // __uart1_h__