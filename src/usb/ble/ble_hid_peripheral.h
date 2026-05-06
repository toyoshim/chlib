// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// HOGP keyboard / pointer-style HID over BLE on top of ble_peripheral.h.

#ifndef __ble_hid_peripheral_h__
#define __ble_hid_peripheral_h__

#include <stdbool.h>
#include <stdint.h>

enum {
  BLE_VID_SOURCE_BT_SIG = 0x01,
  BLE_VID_SOURCE_USB_IF = 0x02,
};

struct ble_pnp_id {
  uint8_t vendor_id_source;     // BLE_VID_SOURCE_*
  uint16_t vendor_id;
  uint16_t product_id;
  uint16_t product_version;     // BCD, e.g. 0x0100 for v1.0
};

struct ble_hid_peripheral {
  const char* local_name;
  uint16_t appearance;
  const char* manufacturer;
  const char* model;
  struct ble_pnp_id pnp_id;

  const uint8_t* report_map;
  uint8_t report_map_len;
  uint8_t report_len;          // size of each input report

  uint32_t passkey;             // 6-digit; 0 → Just Works

  // Battery Service is included in the GATT DB and advertised UUIDs only
  // when this is non-null. The library calls it from
  // ble_hid_peripheral_poll() to refresh the value the central will see;
  // return the level as a percentage (0..100). Cache internally if the
  // measurement is expensive — the library may call this every poll
  // iteration.
  uint8_t (*battery_level)(void);

  // Callbacks (any may be null).
  void (*ready)(void);
  void (*connected)(void);
  void (*disconnected)(void);
  void (*notify_enabled)(bool on);
  void (*sent)(void);
};

void ble_hid_peripheral_init(const struct ble_hid_peripheral* hid,
                             uint8_t usb_host_flags);
void ble_hid_peripheral_poll(void);
// Returns false if a previous transfer is still in flight, the link is not
// yet encrypted, or the central has not enabled notifications.
bool ble_hid_peripheral_send_report(const uint8_t* report, uint8_t len);

#endif  // __ble_hid_peripheral_h__
