// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hci.h"

enum {
  // Command opcodes used internally
  HCI_OPCODE_LE_LTK_REQ_REPLY     = 0x201a,
  HCI_OPCODE_LE_LTK_REQ_NEG_REPLY = 0x201b,

  // Event codes used internally
  HCI_EVT_COMMAND_COMPLETE = 0x0e,
  HCI_EVT_COMMAND_STATUS   = 0x0f,

  // Advertising_Type (LE_Set_Advertising_Parameters)
  HCI_ADV_TYPE_CONN_UNDIRECT    = 0x00,
  HCI_ADV_TYPE_CONN_DIRECT_HIGH = 0x01,
  HCI_ADV_TYPE_SCAN_UNDIRECT    = 0x02,
  HCI_ADV_TYPE_NONCONN_UNDIRECT = 0x03,
  HCI_ADV_TYPE_CONN_DIRECT_LOW  = 0x04,

  // Own_Address_Type / Peer_Address_Type
  HCI_ADDR_TYPE_PUBLIC     = 0x00,
  HCI_ADDR_TYPE_RANDOM     = 0x01,

  // Advertising_Channel_Map (bitmask)
  HCI_ADV_CHAN_ALL = 0x07,

  // Advertising_Filter_Policy
  HCI_ADV_FILTER_ALLOW_ALL = 0x00,
};

struct hci_evt_cmd_complete {
  uint8_t event_code;
  uint8_t plen;
  uint8_t num_cmd_pkt;
  uint16_t opcode;
  uint8_t status;
};

static void fill_event_mask(struct hci_pkt_event_mask* pkt,
                            uint16_t opcode,
                            const uint8_t mask[8]) {
  pkt->header.opcode = opcode;
  pkt->header.plen = sizeof(pkt->mask);
  for (uint8_t i = 0; i < 8; i++) {
    pkt->mask[i] = mask[i];
  }
}

static void fill_adv_payload(struct hci_pkt_le_set_adv_data* pkt,
                             uint16_t opcode,
                             const uint8_t* payload,
                             uint8_t plen) {
  pkt->header.opcode = opcode;
  pkt->header.plen = sizeof(pkt->payload);
  pkt->payload.length = plen;
  for (uint8_t i = 0; i < plen; i++) {
    pkt->payload.data[i] = payload[i];
  }
  for (uint8_t i = plen; i < sizeof(pkt->payload.data); i++) {
    pkt->payload.data[i] = 0;
  }
}

void hci_cmd_reset(struct hci_pkt_reset* pkt) {
  pkt->header.opcode = HCI_OPCODE_RESET;
  pkt->header.plen = 0;
}

void hci_cmd_set_event_mask(struct hci_pkt_event_mask* pkt) {
  static const uint8_t mask[8] = {0xff, 0xff, 0xff, 0xff,
                                  0xff, 0xff, 0xff, 0x3f};
  fill_event_mask(pkt, HCI_OPCODE_SET_EVENT_MASK, mask);
}

void hci_cmd_le_set_event_mask(struct hci_pkt_event_mask* pkt) {
  static const uint8_t mask[8] = {0x1f, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00};
  fill_event_mask(pkt, HCI_OPCODE_LE_SET_EVENT_MASK, mask);
}

void hci_cmd_le_set_adv_params(struct hci_pkt_le_set_adv_params* pkt,
                               uint16_t interval) {
  pkt->header.opcode = HCI_OPCODE_LE_SET_ADV_PARAMS;
  pkt->header.plen = sizeof(pkt->params);
  pkt->params.interval_min = interval;
  pkt->params.interval_max = interval;
  pkt->params.adv_type = HCI_ADV_TYPE_CONN_UNDIRECT;
  pkt->params.own_addr_type = HCI_ADDR_TYPE_PUBLIC;
  pkt->params.peer_addr_type = HCI_ADDR_TYPE_PUBLIC;
  for (uint8_t i = 0; i < sizeof(pkt->params.peer_addr); i++) {
    pkt->params.peer_addr[i] = 0x00;
  }
  pkt->params.channel_map = HCI_ADV_CHAN_ALL;
  pkt->params.filter_policy = HCI_ADV_FILTER_ALLOW_ALL;
}

void hci_cmd_le_set_adv_data(struct hci_pkt_le_set_adv_data* pkt,
                             const uint8_t* payload,
                             uint8_t plen) {
  fill_adv_payload(pkt, HCI_OPCODE_LE_SET_ADV_DATA, payload, plen);
}

void hci_cmd_le_set_scan_rsp_data(struct hci_pkt_le_set_adv_data* pkt,
                                  const uint8_t* payload,
                                  uint8_t plen) {
  fill_adv_payload(pkt, HCI_OPCODE_LE_SET_SCAN_RSP_DATA, payload, plen);
}

void hci_cmd_le_set_adv_enable(struct hci_pkt_le_set_adv_enable* pkt,
                               bool enable) {
  pkt->header.opcode = HCI_OPCODE_LE_SET_ADV_ENABLE;
  pkt->header.plen = sizeof(pkt->enable);
  pkt->enable = enable ? 0x01 : 0x00;
}

void hci_cmd_read_bd_addr(struct hci_pkt_read_bd_addr* pkt) {
  pkt->header.opcode = HCI_OPCODE_READ_BD_ADDR;
  pkt->header.plen = 0;
}

void hci_cmd_le_ltk_req_reply(struct hci_pkt_le_ltk_req_reply* pkt,
                              uint16_t conn_handle,
                              const uint8_t ltk_wire[16]) {
  pkt->header.opcode = HCI_OPCODE_LE_LTK_REQ_REPLY;
  pkt->header.plen = sizeof(pkt->params);
  pkt->params.conn_handle = conn_handle;
  for (uint8_t i = 0; i < 16; i++) {
    pkt->params.ltk[i] = ltk_wire[i];
  }
}

void hci_cmd_le_ltk_req_neg_reply(struct hci_pkt_le_ltk_req_neg_reply* pkt,
                                  uint16_t conn_handle) {
  pkt->header.opcode = HCI_OPCODE_LE_LTK_REQ_NEG_REPLY;
  pkt->header.plen = sizeof(pkt->conn_handle);
  pkt->conn_handle = conn_handle;
}

bool hci_parse_cmd_complete(const uint8_t* data,
                            uint8_t size,
                            uint16_t* opcode,
                            uint8_t* status) {
  const struct hci_evt_cmd_complete* evt =
      (const struct hci_evt_cmd_complete*)data;
  if (size < sizeof(*evt) || evt->event_code != HCI_EVT_COMMAND_COMPLETE) {
    return false;
  }
  *opcode = evt->opcode;
  *status = evt->status;
  return true;
}
