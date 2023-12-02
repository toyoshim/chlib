// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __flash_h__
#define __flash_h__

#include <stdbool.h>
#include <stdint.h>

bool flash_init(uint32_t magic, bool force);
bool flash_write(uint16_t offset, const uint8_t* data, uint16_t size);
bool flash_read(uint16_t offset, uint8_t* data, uint16_t size);

#endif  // __flash_h__