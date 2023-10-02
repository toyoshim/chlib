// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flash.h"

#include "io.h"

static const uint16_t flash_size = 0x0400;
static __code uint8_t* data_flash = (__code uint8_t*)0xf000;
static __code uint8_t* code_flash = (__code uint8_t*)0xec00;
static uint32_t magic = 0;

static inline void enter_safe_mode(void) {
  SAFE_MOD = 0x55;
  SAFE_MOD = 0xaa;
}

static inline void leave_safe_mode(void) {
  SAFE_MOD = 0;
}

static void enter(void) {
  enter_safe_mode();
  GLOBAL_CFG |= bDATA_WE | bCODE_WE;
  leave_safe_mode();
}

static void leave(void) {
  enter_safe_mode();
  GLOBAL_CFG &= ~(bDATA_WE | bCODE_WE);
  leave_safe_mode();
}

static bool erase(__code uint8_t* addr) {
  enter();
  ROM_ADDR_H = (uint16_t)addr >> 8;
  ROM_ADDR_L = (uint16_t)addr;
  ROM_CTRL = 0xa6;
  bool result = ROM_STATUS == 0xc0;
  leave();
  return result;
}

static bool restore(void) {
  if (!erase(data_flash))
    return false;
  enter();
  bool result = true;
  uint16_t addr = (uint16_t)data_flash;
  for (int16_t i = flash_size - 2; i >= 0; i -= 2) {
    ROM_ADDR_H = (addr + i) >> 8;
    ROM_ADDR_L = addr + i;
    ROM_DATA_H = code_flash[i + 1];
    ROM_DATA_L = code_flash[i + 0];
    ROM_CTRL = 0x9a;
    if (ROM_STATUS != 0xc0) {
      result = false;
      break;
    }
  }
  leave();
  return result;
}

bool flash_init(uint32_t new_magic) {
  magic = new_magic;
  __code uint32_t* flash = (__code uint32_t*)data_flash;
  if (magic == *flash)
    return true;
  flash = (__code uint32_t*)code_flash;
  if (magic == *flash) {
    // Maybe the previous shutdown happens during restore operation. Redo.
    return restore();
  }
  // No data. Let's initialize with the magic.
  if (!erase(data_flash))
    return false;
  enter();
  bool result = true;
  ROM_ADDR_H = (uint16_t)data_flash >> 8;
  ROM_ADDR_L = (uint16_t)data_flash;
  ROM_DATA_H = magic >> 8;
  ROM_DATA_L = magic;
  ROM_CTRL = 0x9a;
  if (ROM_STATUS != 0xc0)
    result = false;
  ROM_ADDR_H = (uint16_t)(data_flash + 2) >> 8;
  ROM_ADDR_L = (uint16_t)(data_flash + 2);
  ROM_DATA_H = magic >> 24;
  ROM_DATA_L = magic >> 16;
  ROM_CTRL = 0x9a;
  if (ROM_STATUS != 0xc0)
    result = false;
  leave();
  return result;
}

bool flash_write(uint16_t offset, const uint8_t* data, uint16_t size) {
  // Protect magic region.
  if (offset < 4)
    return false;
  // Prepare block data in the code flash.
  erase(code_flash);
  enter();
  bool result = true;
  // Store the magic last. Thus, data can be consistent if the magic is there.
  for (int16_t i = flash_size - 2; i >= 0; i -= 2) {
    ROM_ADDR_H = (uint16_t)(code_flash + i) >> 8;
    ROM_ADDR_L = (uint16_t)(code_flash + i);
    ROM_DATA_H = (offset <= (i + 1) && (i + 1) < (offset + size))
                     ? data[i + 1 - offset]
                     : data_flash[i + 1];
    ROM_DATA_L =
        (offset <= i && i < (offset + size)) ? data[i - offset] : data_flash[i];
    ROM_CTRL = 0x9a;
    if (ROM_STATUS != 0xc0) {
      result = false;
      break;
    }
  }
  leave();
  // Now the backup in the code flash works. Let's copy it to the data flash.
  // Initialization code can restore data from the backup if this operation
  // does not complete due to unexpected shutdown.
  restore();
  return result;
}

bool flash_read(uint16_t offset, uint8_t* data, uint16_t size) {
  __code uint32_t* flash = (__code uint32_t*)data_flash;
  if (magic != *flash)
    return false;
  for (uint16_t i = 0; i < size; ++i)
    data[i] = data_flash[offset + i];
  return true;
}