// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Peripheral-side BLE on top of chlib's usb_host. Drives the HCI bring-up
// sequence (Reset → Set_Event_Mask → LE_Set_Event_Mask → Read_BD_ADDR →
// LE_Set_Advertising_Parameters → LE_Set_Advertising_Data →
// LE_Set_Scan_Response_Data → LE_Set_Advertising_Enable) and surfaces
// connection lifecycle to the caller.

#ifndef __ble_h__
#define __ble_h__

#include <stdbool.h>
#include <stdint.h>

struct ble {
  uint8_t usb_host_flags;

  // Advertising configuration. Library copies these into HCI commands.
  uint16_t adv_interval;          // 0.625ms units; 0 → use 100ms default
  const uint8_t* adv_data;
  uint8_t adv_data_len;
  const uint8_t* scan_rsp_data;
  uint8_t scan_rsp_data_len;

  // 6-digit Legacy Passkey. 0 selects Just Works (no MITM protection).
  uint32_t passkey;

  // Lifecycle callbacks. Any may be null. `acl` receives non-SMP L2CAP
  // payloads (the library handles SMP internally).
  void (*advertising)(void);
  void (*connected)(uint16_t handle);
  void (*disconnected)(void);
  void (*encrypted)(bool on);
  void (*acl)(const uint8_t* data, uint16_t size);
  void (*sent)(void);
};

void ble_init(struct ble* ble);
void ble_poll(void);
// Transfers are serialized; wait for `sent` before issuing the next.
bool ble_send_hci_cmd(const void* buf, uint8_t len);
bool ble_send_acl(const void* buf, uint8_t len);

#endif  // __ble_h__
