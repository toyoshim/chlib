// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// HCI command/event encoders and parsers, transport agnostic.

#ifndef __ble_hci_h__
#define __ble_hci_h__

#include <stdbool.h>
#include <stdint.h>

enum {
  // command opcodes (OGF << 10 | OCF, BT spec Vol 4 Part E sec. 7)
  HCI_OPCODE_SET_EVENT_MASK       = 0x0c01,
  HCI_OPCODE_RESET                = 0x0c03,
  HCI_OPCODE_READ_BD_ADDR         = 0x1009,
  HCI_OPCODE_LE_SET_EVENT_MASK    = 0x2001,
  HCI_OPCODE_LE_SET_ADV_PARAMS    = 0x2006,
  HCI_OPCODE_LE_SET_ADV_DATA      = 0x2008,
  HCI_OPCODE_LE_SET_SCAN_RSP_DATA = 0x2009,
  HCI_OPCODE_LE_SET_ADV_ENABLE    = 0x200a,

  // event codes (BT spec Vol 4 Part E sec. 7.7)
  HCI_EVT_DISCONNECTION_COMPLETE = 0x05,
  HCI_EVT_ENCRYPTION_CHANGE      = 0x08,
  HCI_EVT_LE_META                = 0x3e,

  // LE Meta sub-events
  HCI_LE_SUBEVT_CONNECTION_COMPLETE = 0x01,
  HCI_LE_SUBEVT_LTK_REQUEST         = 0x05,

  // AD types (GAP, Core Spec Supplement) carried in advertising payloads
  HCI_AD_FLAGS               = 0x01,
  HCI_AD_UUID16_COMPLETE     = 0x03,
  HCI_AD_LOCAL_NAME_COMPLETE = 0x09,
  HCI_AD_APPEARANCE          = 0x19,

  // AD Flags payload bits (HCI_AD_FLAGS)
  HCI_AD_FLAG_LE_GENERAL_DISC      = 1 << 1,
  HCI_AD_FLAG_BR_EDR_NOT_SUPPORTED = 1 << 2,
};

struct hci_cmd_header {
  uint16_t opcode;
  uint8_t plen;
};

struct hci_le_adv_params {
  uint16_t interval_min;
  uint16_t interval_max;
  uint8_t adv_type;
  uint8_t own_addr_type;
  uint8_t peer_addr_type;
  uint8_t peer_addr[6];
  uint8_t channel_map;
  uint8_t filter_policy;
};

// LE_Set_Advertising_Data / LE_Set_Scan_Response_Data parameter block.
struct hci_le_adv_payload {
  uint8_t length;
  uint8_t data[31];
};

struct hci_pkt_reset {
  struct hci_cmd_header header;
};

// Shared by HCI_Set_Event_Mask and HCI_LE_Set_Event_Mask.
struct hci_pkt_event_mask {
  struct hci_cmd_header header;
  uint8_t mask[8];
};

struct hci_pkt_le_set_adv_params {
  struct hci_cmd_header header;
  struct hci_le_adv_params params;
};

// Used for both LE_Set_Advertising_Data and LE_Set_Scan_Response_Data.
struct hci_pkt_le_set_adv_data {
  struct hci_cmd_header header;
  struct hci_le_adv_payload payload;
};

struct hci_pkt_le_set_adv_enable {
  struct hci_cmd_header header;
  uint8_t enable;
};

struct hci_pkt_read_bd_addr {
  struct hci_cmd_header header;
};

struct hci_le_ltk_reply_params {
  uint16_t conn_handle;
  uint8_t ltk[16];
};

struct hci_pkt_le_ltk_req_reply {
  struct hci_cmd_header header;
  struct hci_le_ltk_reply_params params;
};

struct hci_pkt_le_ltk_req_neg_reply {
  struct hci_cmd_header header;
  uint16_t conn_handle;
};

void hci_cmd_reset(struct hci_pkt_reset* pkt);
void hci_cmd_set_event_mask(struct hci_pkt_event_mask* pkt);
void hci_cmd_le_set_event_mask(struct hci_pkt_event_mask* pkt);
void hci_cmd_le_set_adv_params(struct hci_pkt_le_set_adv_params* pkt,
                               uint16_t interval);
void hci_cmd_le_set_adv_data(struct hci_pkt_le_set_adv_data* pkt,
                             const uint8_t* payload,
                             uint8_t plen);
void hci_cmd_le_set_scan_rsp_data(struct hci_pkt_le_set_adv_data* pkt,
                                  const uint8_t* payload,
                                  uint8_t plen);
void hci_cmd_le_set_adv_enable(struct hci_pkt_le_set_adv_enable* pkt,
                               bool enable);
void hci_cmd_read_bd_addr(struct hci_pkt_read_bd_addr* pkt);
void hci_cmd_le_ltk_req_reply(struct hci_pkt_le_ltk_req_reply* pkt,
                              uint16_t conn_handle,
                              const uint8_t ltk_wire[16]);
void hci_cmd_le_ltk_req_neg_reply(struct hci_pkt_le_ltk_req_neg_reply* pkt,
                                  uint16_t conn_handle);

bool hci_parse_cmd_complete(const uint8_t* data,
                            uint8_t size,
                            uint16_t* opcode,
                            uint8_t* status);

#endif  // __ble_hci_h__
