// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ble.h"

#include "../usb.h"
#include "../usb_host.h"

enum {
  VID_BROADCOM = 0x0a5c,
  EP_EVENT     = 1,
  EP_ACL       = 2,
  IN_REQ_SIZE  = 64,
};

enum {
  STATE_DISCONNECTED,
  STATE_DETECTED,
  STATE_POLL_EVENT,
  STATE_POLL_ACL,
  STATE_SEND_HCI,
  STATE_SEND_ACL,
};

static struct ble* ble;
static struct usb_host host;
static uint8_t state;
static uint8_t hub_in_use;
static uint16_t candidate_vid[2];
static const void* tx_buf;
static uint8_t tx_len;

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

static void disconnected(uint8_t hub) {
  if (hub != hub_in_use) {
    return;
  }
  state = STATE_DISCONNECTED;
  if (ble->disconnected) {
    ble->disconnected();
  }
}

static void check_device_desc(uint8_t hub, const uint8_t* data) {
  const struct usb_desc_device* desc = (const struct usb_desc_device*)data;
  candidate_vid[hub] = desc->idVendor;
  // Already claimed a dongle on another hub; ignore to avoid takeover.
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

static void in_cb(uint8_t hub, uint8_t ep, uint8_t* data, uint16_t size) {
  if (hub != hub_in_use || size == 0) {
    return;
  }
  if (ep == EP_EVENT && ble->event) {
    ble->event(data, (uint8_t)size);
  } else if (ep == EP_ACL && ble->acl) {
    ble->acl(data, size);
  }
}

void ble_init(struct ble* config) {
  ble = config;
  state = STATE_DISCONNECTED;
  hub_in_use = 0;

  host.flags = config->usb_host_flags;
  host.disconnected = disconnected;
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

bool ble_send_hci_cmd(const void* buf, uint8_t len) {
  return stage_tx(STATE_SEND_HCI, buf, len);
}

bool ble_send_acl(const void* buf, uint8_t len) {
  return stage_tx(STATE_SEND_ACL, buf, len);
}

void ble_poll(void) {
  usb_host_poll();
  if (!usb_host_idle()) {
    return;
  }
  switch (state) {
    case STATE_DETECTED:
      if (usb_host_ready(hub_in_use)) {
        state = STATE_POLL_EVENT;
        if (ble->ready) {
          ble->ready();
        }
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
        if (ble->sent) {
          ble->sent();
        }
      }
      break;
    }
    case STATE_SEND_ACL:
      if (usb_host_out(hub_in_use, EP_ACL, (uint8_t*)tx_buf, tx_len)) {
        state = STATE_POLL_EVENT;
        if (ble->sent) {
          ble->sent();
        }
      }
      break;
  }
}
