// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Bluetooth HCI USB transport over chlib's usb_host.

#ifndef __ble_h__
#define __ble_h__

#include <stdbool.h>
#include <stdint.h>

struct ble {
  uint8_t usb_host_flags;
  void (*event)(const uint8_t* data, uint8_t size);
  void (*acl)(const uint8_t* data, uint16_t size);
  void (*ready)(void);
  void (*disconnected)(void);
  void (*sent)(void);
};

void ble_init(struct ble* ble);
void ble_poll(void);
// Transfers are serialized; wait for `sent` before issuing the next.
bool ble_send_hci_cmd(const void* buf, uint8_t len);
bool ble_send_acl(const void* buf, uint8_t len);

#endif  // __ble_h__
