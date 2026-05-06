// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "l2cap.h"

// ACL handle_flags bits 0-11: connection handle; bits 12-13: PB; 14-15: BC.
struct hci_acl_header {
  uint16_t handle_flags;
  uint16_t length;
};

struct l2cap_header {
  uint16_t length;
  uint16_t cid;
};

bool l2cap_parse(const uint8_t* data, uint16_t size, struct l2cap_pkt* out) {
  if (size < sizeof(struct hci_acl_header) + sizeof(struct l2cap_header)) {
    return false;
  }
  const struct hci_acl_header* acl = (const struct hci_acl_header*)data;
  const struct l2cap_header* l2 =
      (const struct l2cap_header*)(data + sizeof(*acl));
  uint16_t hdr_len = sizeof(*acl) + sizeof(*l2);
  if (size < hdr_len + l2->length) {
    return false;
  }
  out->handle = acl->handle_flags & 0x0fff;
  out->cid = l2->cid;
  out->payload = data + hdr_len;
  out->payload_len = l2->length;
  return true;
}

uint16_t l2cap_build(uint8_t* out,
                     uint16_t handle,
                     uint16_t cid,
                     const void* payload,
                     uint16_t payload_len) {
  struct hci_acl_header* acl = (struct hci_acl_header*)out;
  struct l2cap_header* l2 = (struct l2cap_header*)(out + sizeof(*acl));
  // PB = 0b10 (First auto-flushable LE packet, BT spec Vol 4 Part E sec 5.4.2),
  // BC = 0b00 (point-to-point).
  acl->handle_flags = (handle & 0x0fff) | (0x2 << 12);
  acl->length = sizeof(*l2) + payload_len;
  l2->length = payload_len;
  l2->cid = cid;
  uint8_t* dst = out + sizeof(*acl) + sizeof(*l2);
  const uint8_t* src = (const uint8_t*)payload;
  for (uint16_t i = 0; i < payload_len; i++) {
    dst[i] = src[i];
  }
  return sizeof(*acl) + sizeof(*l2) + payload_len;
}
