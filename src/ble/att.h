// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ATT server over a static GATT DB.

#ifndef __ble_att_h__
#define __ble_att_h__

#include <stdbool.h>
#include <stdint.h>

enum {
  // ATT opcodes used by callers (BT spec Vol 3 Part F sec. 3.4)
  ATT_OP_ERROR_RSP        = 0x01,
  ATT_OP_EXCHANGE_MTU_REQ = 0x02,
  ATT_OP_EXCHANGE_MTU_RSP = 0x03,
  ATT_OP_WRITE_REQ        = 0x12,
  ATT_OP_WRITE_RSP        = 0x13,
  ATT_OP_HANDLE_VALUE_NTF = 0x1b,
  ATT_OP_WRITE_CMD        = 0x52,

  // GATT declaration UUIDs used in DB definitions
  UUID_PRIMARY_SERVICE = 0x2800,
  UUID_CHARACTERISTIC  = 0x2803,

  // GATT descriptor UUIDs used in DB definitions
  UUID_CCCD             = 0x2902,
  UUID_REPORT_REFERENCE = 0x2908,

  // characteristic property bits (in characteristic declaration value)
  CHAR_PROP_READ         = 1 << 1,
  CHAR_PROP_WRITE_NO_RSP = 1 << 2,
  CHAR_PROP_NOTIFY       = 1 << 4,

  // ble_attr.flags
  BLE_ATTR_WRITABLE  = 1 << 0,
  BLE_ATTR_ENCRYPTED = 1 << 1,
};

struct ble_attr {
  uint16_t handle;
  uint16_t type_uuid;
  uint8_t* value;
  uint8_t value_len;
  uint8_t flags;
};

struct att_ctx {
  const struct ble_attr* db;
  uint8_t db_count;
  uint8_t mtu;
  bool encrypted;
};

// Returns the number of response bytes written to `out`, or 0 when the
// request needs no reply (e.g. Write Command).
uint8_t att_dispatch(const struct att_ctx* ctx,
                     const uint8_t* req,
                     uint8_t req_len,
                     uint8_t* out,
                     uint8_t out_max);

#endif  // __ble_att_h__
