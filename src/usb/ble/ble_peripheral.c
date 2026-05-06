// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ble_peripheral.h"

#include "../../ble/hci.h"
#include "../../ble/l2cap.h"
#include "../../ble/smp.h"
#include "../usb.h"
#include "../usb_host.h"

enum {
  VID_BROADCOM = 0x0a5c,
  EP_EVENT     = 1,
  EP_ACL       = 2,
  IN_REQ_SIZE  = 64,

  // 100ms in 0.625ms units, used when caller leaves adv_interval unset.
  ADV_INTERVAL_DEFAULT = 0x00a0,

  // Default ATT MTU per spec; the only one this library negotiates.
  ATT_MTU = 23,

  // ACL TX scratch: 4 ACL + 4 L2CAP + max ATT MTU payload.
  ACL_TX_BUF_SIZE = 8 + ATT_MTU,
};

enum {
  STATE_DISCONNECTED,
  STATE_DETECTED,
  STATE_POLL_EVENT,
  STATE_POLL_ACL,
  STATE_SEND_HCI,
  STATE_SEND_ACL,
};

enum {
  BRINGUP_IDLE = 0,
  BRINGUP_RESET,
  BRINGUP_SET_EVENT_MASK,
  BRINGUP_LE_SET_EVENT_MASK,
  BRINGUP_READ_BD_ADDR,
  BRINGUP_ADV_PARAMS,
  BRINGUP_ADV_DATA,
  BRINGUP_SCAN_RSP,
  BRINGUP_ADV_ENABLE,
  BRINGUP_DONE,
};

static const uint16_t bringup_opcode[] = {
    0,  // BRINGUP_IDLE placeholder
    HCI_OPCODE_RESET,
    HCI_OPCODE_SET_EVENT_MASK,
    HCI_OPCODE_LE_SET_EVENT_MASK,
    HCI_OPCODE_READ_BD_ADDR,
    HCI_OPCODE_LE_SET_ADV_PARAMS,
    HCI_OPCODE_LE_SET_ADV_DATA,
    HCI_OPCODE_LE_SET_SCAN_RSP_DATA,
    HCI_OPCODE_LE_SET_ADV_ENABLE,
};

static struct ble_peripheral* ble;
static struct usb_host host;
static uint8_t state;
static uint8_t hub_in_use;
static uint16_t candidate_vid[2];
static const void* tx_buf;
static uint8_t tx_len;
static uint8_t bringup_step;
static uint8_t own_addr_wire[6];

// SMP/connection state owned by the library.
static struct smp_session smp;
static uint16_t conn_handle;
static bool encrypted_state;
static bool smp_phase3_pending;
static uint8_t smp_phase3_pdu[17];
static uint8_t smp_phase3_pdu_len;
static uint8_t acl_tx_buf[ACL_TX_BUF_SIZE];
static uint8_t current_mtu;

static bool ble_send_hci_cmd(const void* buf, uint8_t len);
static bool ble_send_acl(const void* buf, uint8_t len);

static union {
  struct hci_pkt_reset reset;
  struct hci_pkt_event_mask event_mask;
  struct hci_pkt_read_bd_addr read_bd_addr;
  struct hci_pkt_le_set_adv_params adv_params;
  struct hci_pkt_le_set_adv_data adv_data;
  struct hci_pkt_le_set_adv_enable adv_enable;
  struct hci_pkt_le_ltk_req_reply ltk_reply;
  struct hci_pkt_le_ltk_req_neg_reply ltk_neg_reply;
} hci_pkt;

static bool is_bt_iface(uint8_t cls, uint8_t sub, uint8_t proto, uint16_t vid) {
  if (sub != USB_WIRELESS_SUBCLASS_RF ||
      proto != USB_WIRELESS_PROTOCOL_BLUETOOTH) {
    return false;
  }
  if (cls == USB_CLASS_WIRELESS) {
    return true;
  }
  // Vendor-specific is also used by non-BT devices; only Broadcom BCM20702A0.
  if (cls == USB_CLASS_VENDOR) {
    return vid == VID_BROADCOM;
  }
  return false;
}

static void check_device_desc(uint8_t hub, const uint8_t* data) {
  const struct usb_desc_device* desc = (const struct usb_desc_device*)data;
  candidate_vid[hub] = desc->idVendor;
  if (state != STATE_DISCONNECTED) {
    return;
  }
  if (is_bt_iface(desc->bDeviceClass, desc->bDeviceSubClass,
                  desc->bDeviceProtocol, candidate_vid[hub])) {
    hub_in_use = hub;
    state = STATE_DETECTED;
  }
}

static uint8_t check_configuration_desc(uint8_t hub, const uint8_t* data) {
  if (state != STATE_DISCONNECTED && hub != hub_in_use) {
    return 0xff;
  }
  const struct usb_desc_configuration* cfg =
      (const struct usb_desc_configuration*)data;
  const struct usb_desc_head* head = (const struct usb_desc_head*)data;
  uint8_t target = 0xff;
  for (uint16_t off = head->bLength; off < cfg->wTotalLength;) {
    head = (const struct usb_desc_head*)(data + off);
    if (head->bDescriptorType == USB_DESC_INTERFACE) {
      const struct usb_desc_interface* intf =
          (const struct usb_desc_interface*)(data + off);
      if (is_bt_iface(intf->bInterfaceClass, intf->bInterfaceSubClass,
                      intf->bInterfaceProtocol, candidate_vid[hub])) {
        hub_in_use = hub;
        state = STATE_DETECTED;
        target = intf->bInterfaceNumber;
        break;
      }
    }
    off += head->bLength;
  }
  return target;
}

static void send_bringup_step(void) {
  uint16_t interval = ble->adv_interval ? ble->adv_interval
                                        : ADV_INTERVAL_DEFAULT;
  switch (bringup_step) {
    case BRINGUP_RESET:
      hci_cmd_reset(&hci_pkt.reset);
      ble_send_hci_cmd(&hci_pkt.reset, sizeof(hci_pkt.reset));
      break;
    case BRINGUP_SET_EVENT_MASK:
      hci_cmd_set_event_mask(&hci_pkt.event_mask);
      ble_send_hci_cmd(&hci_pkt.event_mask, sizeof(hci_pkt.event_mask));
      break;
    case BRINGUP_LE_SET_EVENT_MASK:
      hci_cmd_le_set_event_mask(&hci_pkt.event_mask);
      ble_send_hci_cmd(&hci_pkt.event_mask, sizeof(hci_pkt.event_mask));
      break;
    case BRINGUP_READ_BD_ADDR:
      hci_cmd_read_bd_addr(&hci_pkt.read_bd_addr);
      ble_send_hci_cmd(&hci_pkt.read_bd_addr, sizeof(hci_pkt.read_bd_addr));
      break;
    case BRINGUP_ADV_PARAMS:
      hci_cmd_le_set_adv_params(&hci_pkt.adv_params, interval);
      ble_send_hci_cmd(&hci_pkt.adv_params, sizeof(hci_pkt.adv_params));
      break;
    case BRINGUP_ADV_DATA:
      hci_cmd_le_set_adv_data(&hci_pkt.adv_data, ble->adv_data,
                              ble->adv_data_len);
      ble_send_hci_cmd(&hci_pkt.adv_data, sizeof(hci_pkt.adv_data));
      break;
    case BRINGUP_SCAN_RSP:
      hci_cmd_le_set_scan_rsp_data(&hci_pkt.adv_data, ble->scan_rsp_data,
                                   ble->scan_rsp_data_len);
      ble_send_hci_cmd(&hci_pkt.adv_data, sizeof(hci_pkt.adv_data));
      break;
    case BRINGUP_ADV_ENABLE:
      hci_cmd_le_set_adv_enable(&hci_pkt.adv_enable, true);
      ble_send_hci_cmd(&hci_pkt.adv_enable, sizeof(hci_pkt.adv_enable));
      break;
  }
}

static void send_security_request(void) {
  uint8_t pdu[2];
  pdu[0] = SMP_OP_SECURITY_REQUEST;
  pdu[1] = SMP_AUTH_BONDING | (ble->passkey ? SMP_AUTH_MITM : 0);
  uint16_t total = l2cap_build(acl_tx_buf, conn_handle, L2CAP_CID_SMP,
                               pdu, sizeof(pdu));
  ble_send_acl(acl_tx_buf, (uint8_t)total);
}

static void pump_phase3(void) {
  if (smp_phase3_pdu_len == 0) {
    smp_phase3_pdu_len = smp_next_phase3_pdu(&smp, smp_phase3_pdu,
                                             sizeof(smp_phase3_pdu));
    if (smp_phase3_pdu_len == 0) {
      smp_phase3_pending = false;
      return;
    }
  }
  uint16_t total = l2cap_build(acl_tx_buf, conn_handle, L2CAP_CID_SMP,
                               smp_phase3_pdu, smp_phase3_pdu_len);
  if (ble_send_acl(acl_tx_buf, (uint8_t)total)) {
    smp_phase3_pdu_len = 0;
  }
}

static void dispatch_smp(const struct l2cap_pkt* l2) {
  uint8_t rsp[17];
  uint8_t rsp_len = smp_handle_pdu(&smp, l2->payload,
                                   (uint8_t)l2->payload_len,
                                   rsp, sizeof(rsp));
  if (rsp_len == 0) {
    return;
  }
  uint16_t total = l2cap_build(acl_tx_buf, conn_handle, L2CAP_CID_SMP,
                               rsp, rsp_len);
  ble_send_acl(acl_tx_buf, (uint8_t)total);
}

static void handle_le_ltk_request(const uint8_t* data, uint8_t size) {
  // Layout: 0x3e plen 0x05 handle(2) rand(8) ediv(2) → size = 15.
  if (size < 15) {
    return;
  }
  uint16_t handle = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
  const uint8_t* rand_wire = &data[5];
  const uint8_t* ediv_le = &data[13];
  uint8_t ltk_wire[16];
  bool have_ltk = false;
  // Phase 2 STK: ediv = 0, rand = 0.
  if (ediv_le[0] == 0 && ediv_le[1] == 0) {
    bool zero_rand = true;
    for (uint8_t i = 0; i < 8; i++) {
      if (rand_wire[i] != 0) {
        zero_rand = false;
        break;
      }
    }
    if (zero_rand && smp.state == SMP_STATE_STK_READY) {
      smp_get_stk_wire(&smp, ltk_wire);
      have_ltk = true;
    }
  }
  if (!have_ltk) {
    have_ltk = smp_lookup_ltk(&smp, ediv_le, rand_wire, ltk_wire);
  }
  if (have_ltk) {
    hci_cmd_le_ltk_req_reply(&hci_pkt.ltk_reply, handle, ltk_wire);
    ble_send_hci_cmd(&hci_pkt.ltk_reply, sizeof(hci_pkt.ltk_reply));
  } else {
    hci_cmd_le_ltk_req_neg_reply(&hci_pkt.ltk_neg_reply, handle);
    ble_send_hci_cmd(&hci_pkt.ltk_neg_reply, sizeof(hci_pkt.ltk_neg_reply));
  }
}

static void handle_encryption_change(const uint8_t* data, uint8_t size) {
  // Layout: 0x08 plen=4 status handle(2) enabled.
  if (size < 6 || data[2] != 0x00) {
    return;
  }
  encrypted_state = (data[5] != 0);
  if (encrypted_state) {
    smp_phase3_pending = true;
    smp_phase3_pdu_len = 0;
  }
  if (ble->encrypted) {
    ble->encrypted(encrypted_state);
  }
  if (encrypted_state) {
    pump_phase3();
  }
}

static void on_le_connection_complete(const uint8_t* data, uint8_t size) {
  if (size < 14 || data[3] != 0x00) {
    return;
  }
  conn_handle = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
  encrypted_state = false;
  smp_phase3_pending = false;
  smp_phase3_pdu_len = 0;
  current_mtu = ATT_MTU;
  smp_init(&smp, &data[8], data[7], own_addr_wire, 0, ble->passkey);
  if (ble->connected) {
    ble->connected(conn_handle);
  }
  // Apple BAD R20 sec. 35.4: HID accessories must send Security Request so
  // the central initiates pairing.
  send_security_request();
}

static void handle_event(const uint8_t* data, uint8_t size) {
  if (size < 1) {
    return;
  }
  if (bringup_step >= BRINGUP_RESET && bringup_step <= BRINGUP_ADV_ENABLE &&
      data[0] == HCI_EVT_COMMAND_COMPLETE) {
    uint16_t opcode;
    uint8_t status;
    if (hci_parse_cmd_complete(data, size, &opcode, &status) &&
        opcode == bringup_opcode[bringup_step] && status == 0x00) {
      // Read_BD_ADDR Command Complete carries BD_ADDR (wire) in bytes 6..11.
      if (bringup_step == BRINGUP_READ_BD_ADDR && size >= 12) {
        for (uint8_t i = 0; i < 6; i++) {
          own_addr_wire[i] = data[6 + i];
        }
      }
      if (bringup_step == BRINGUP_ADV_ENABLE) {
        bringup_step = BRINGUP_DONE;
        if (ble->advertising) {
          ble->advertising();
        }
      } else {
        bringup_step++;
        send_bringup_step();
      }
      return;
    }
  }
  if (data[0] == HCI_EVT_LE_META && size >= 3) {
    if (data[2] == HCI_LE_SUBEVT_CONNECTION_COMPLETE) {
      on_le_connection_complete(data, size);
    } else if (data[2] == HCI_LE_SUBEVT_LTK_REQUEST) {
      handle_le_ltk_request(data, size);
    }
  } else if (data[0] == HCI_EVT_DISCONNECTION_COMPLETE) {
    encrypted_state = false;
    smp_phase3_pending = false;
    smp_phase3_pdu_len = 0;
    bringup_step = BRINGUP_ADV_ENABLE;
    send_bringup_step();
    if (ble->disconnected) {
      ble->disconnected();
    }
  } else if (data[0] == HCI_EVT_ENCRYPTION_CHANGE) {
    handle_encryption_change(data, size);
  }
}

static void dispatch_att(const struct l2cap_pkt* l2) {
  uint8_t att_rsp[ATT_MTU];
  struct att_ctx ctx = {ble->db, ble->db_count, current_mtu, encrypted_state};
  uint8_t rsp_len = att_dispatch(&ctx, l2->payload,
                                 (uint8_t)l2->payload_len,
                                 att_rsp, sizeof(att_rsp));
  if (rsp_len) {
    if (l2->payload[0] == ATT_OP_EXCHANGE_MTU_REQ &&
        att_rsp[0] == ATT_OP_EXCHANGE_MTU_RSP) {
      uint16_t client_mtu = (uint16_t)l2->payload[1] |
                            ((uint16_t)l2->payload[2] << 8);
      current_mtu = client_mtu < ATT_MTU ? (uint8_t)client_mtu : ATT_MTU;
    }
    uint16_t total = l2cap_build(acl_tx_buf, conn_handle, L2CAP_CID_ATT,
                                 att_rsp, rsp_len);
    ble_send_acl(acl_tx_buf, (uint8_t)total);
  }
  bool wrote = (l2->payload[0] == ATT_OP_WRITE_REQ &&
                rsp_len > 0 && att_rsp[0] == ATT_OP_WRITE_RSP) ||
               (l2->payload[0] == ATT_OP_WRITE_CMD && rsp_len == 0);
  if (wrote && l2->payload_len >= 3 && ble->attr_write) {
    uint16_t handle = (uint16_t)l2->payload[1] |
                      ((uint16_t)l2->payload[2] << 8);
    ble->attr_write(handle, &l2->payload[3],
                    (uint8_t)(l2->payload_len - 3));
  }
}

static void handle_acl(const uint8_t* data, uint16_t size) {
  struct l2cap_pkt l2;
  if (!l2cap_parse(data, size, &l2)) {
    return;
  }
  if (l2.cid == L2CAP_CID_SMP) {
    dispatch_smp(&l2);
  } else if (l2.cid == L2CAP_CID_ATT) {
    dispatch_att(&l2);
  }
}

static void on_usb_disconnected(uint8_t hub) {
  if (hub != hub_in_use) {
    return;
  }
  state = STATE_DISCONNECTED;
  bringup_step = BRINGUP_IDLE;
  encrypted_state = false;
  smp_phase3_pending = false;
  smp_phase3_pdu_len = 0;
  if (ble->disconnected) {
    ble->disconnected();
  }
}

static void in_cb(uint8_t hub, uint8_t ep, uint8_t* data, uint16_t size) {
  if (hub != hub_in_use || size == 0) {
    return;
  }
  if (ep == EP_EVENT) {
    handle_event(data, (uint8_t)size);
  } else if (ep == EP_ACL) {
    handle_acl(data, size);
  }
}

void ble_peripheral_init(struct ble_peripheral* config) {
  ble = config;
  state = STATE_DISCONNECTED;
  hub_in_use = 0;
  bringup_step = BRINGUP_IDLE;
  encrypted_state = false;
  smp_phase3_pending = false;
  smp_phase3_pdu_len = 0;

  host.flags = config->usb_host_flags;
  host.disconnected = on_usb_disconnected;
  host.check_device_desc = check_device_desc;
  host.check_string_desc = 0;
  host.check_configuration_desc = check_configuration_desc;
  host.check_hid_report_desc = 0;
  host.in = in_cb;
  host.hid_report = 0;
  usb_host_init(&host);
}

static bool stage_tx(uint8_t next, const void* buf, uint8_t len) {
  if (state != STATE_POLL_EVENT && state != STATE_POLL_ACL) {
    return false;
  }
  tx_buf = buf;
  tx_len = len;
  state = next;
  return true;
}

static bool ble_send_hci_cmd(const void* buf, uint8_t len) {
  return stage_tx(STATE_SEND_HCI, buf, len);
}

static bool ble_send_acl(const void* buf, uint8_t len) {
  return stage_tx(STATE_SEND_ACL, buf, len);
}

bool ble_peripheral_send_notification(uint16_t handle,
                           const uint8_t* value,
                           uint8_t value_len) {
  // Notification PDU: opcode + handle(2) + value, framed in L2CAP/ACL.
  if ((uint16_t)value_len + 8 + 3 > sizeof(acl_tx_buf)) {
    return false;
  }
  uint8_t pdu[3 + ATT_MTU];
  pdu[0] = ATT_OP_HANDLE_VALUE_NTF;
  pdu[1] = handle & 0xff;
  pdu[2] = handle >> 8;
  for (uint8_t i = 0; i < value_len; i++) {
    pdu[3 + i] = value[i];
  }
  uint16_t total = l2cap_build(acl_tx_buf, conn_handle, L2CAP_CID_ATT,
                               pdu, (uint16_t)(3 + value_len));
  return ble_send_acl(acl_tx_buf, (uint8_t)total);
}

// Drives Phase 3 first (library work); falls through to caller's `sent` only
// when the library has nothing pending.
static void on_internal_sent(void) {
  if (smp_phase3_pending && encrypted_state) {
    pump_phase3();
    return;
  }
  if (ble->sent) {
    ble->sent();
  }
}

void ble_peripheral_poll(void) {
  usb_host_poll();
  if (!usb_host_idle()) {
    return;
  }
  switch (state) {
    case STATE_DETECTED:
      if (usb_host_ready(hub_in_use)) {
        state = STATE_POLL_EVENT;
        bringup_step = BRINGUP_RESET;
        send_bringup_step();
      }
      break;
    case STATE_POLL_EVENT:
      if (usb_host_in(hub_in_use, EP_EVENT, IN_REQ_SIZE)) {
        state = STATE_POLL_ACL;
      }
      break;
    case STATE_POLL_ACL:
      if (usb_host_in(hub_in_use, EP_ACL, IN_REQ_SIZE)) {
        state = STATE_POLL_EVENT;
      }
      break;
    case STATE_SEND_HCI: {
      const struct usb_setup_req setup = {
          USB_REQ_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_DEVICE,
          0, 0, 0, tx_len,
      };
      if (usb_host_setup(hub_in_use, &setup, tx_buf)) {
        state = STATE_POLL_EVENT;
        on_internal_sent();
      }
      break;
    }
    case STATE_SEND_ACL:
      if (usb_host_out(hub_in_use, EP_ACL, (uint8_t*)tx_buf, tx_len)) {
        state = STATE_POLL_EVENT;
        on_internal_sent();
      }
      break;
  }
}
