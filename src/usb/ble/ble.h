// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Peripheral-side BLE on top of chlib's usb_host: HCI bring-up, SMP, and
// ATT against a caller-supplied GATT DB.

#ifndef __ble_h__
#define __ble_h__

#include <stdbool.h>
#include <stdint.h>

#include "../../ble/att.h"

struct ble {
  uint8_t usb_host_flags;

  uint16_t adv_interval;          // 0.625ms units; 0 → 100ms default
  const uint8_t* adv_data;
  uint8_t adv_data_len;
  const uint8_t* scan_rsp_data;
  uint8_t scan_rsp_data_len;

  uint32_t passkey;               // 6-digit; 0 → Just Works

  const struct ble_attr* db;
  uint8_t db_count;

  // Callbacks (any may be null).
  void (*advertising)(void);
  void (*connected)(uint16_t handle);
  void (*disconnected)(void);
  void (*encrypted)(bool on);
  // Fired after a successful ATT write to a writable attribute.
  void (*attr_write)(uint16_t handle,
                     const uint8_t* value,
                     uint8_t value_len);
  void (*sent)(void);
};

void ble_init(struct ble* ble);
void ble_poll(void);
// Returns false while a previous transfer is in flight; wait for `sent`.
bool ble_send_notification(uint16_t handle,
                           const uint8_t* value,
                           uint8_t value_len);

#endif  // __ble_h__
