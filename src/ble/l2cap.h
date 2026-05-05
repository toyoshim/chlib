// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// HCI ACL + L2CAP framing helpers.

#ifndef __ble_l2cap_h__
#define __ble_l2cap_h__

#include <stdbool.h>
#include <stdint.h>

enum {
  L2CAP_CID_ATT = 0x0004,
  L2CAP_CID_SMP = 0x0006,
};

struct l2cap_pkt {
  uint16_t handle;
  uint16_t cid;
  const uint8_t* payload;
  uint16_t payload_len;
};

// On success, `out->payload` points inside `data` (not copied).
bool l2cap_parse(const uint8_t* data, uint16_t size, struct l2cap_pkt* out);

// Caller must size `out` for 8 + payload_len bytes (4 ACL + 4 L2CAP + payload).
uint16_t l2cap_build(uint8_t* out,
                     uint16_t handle,
                     uint16_t cid,
                     const void* payload,
                     uint16_t payload_len);

#endif  // __ble_l2cap_h__
