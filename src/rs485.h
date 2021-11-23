// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __rs485_h__
#define __rs485_h__

#include <stdbool.h>
#include <stdint.h>

// note: pins are fixed to XA/XB for now
void rs485_init();
void rs485_send(uint8_t val);
bool rs485_ready();
uint8_t rs485_recv();

#endif  // __rs485_h__