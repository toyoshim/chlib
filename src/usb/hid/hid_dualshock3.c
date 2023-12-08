// Copyright 2022 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "hid_dualshock3.h"

#include "../usb.h"
#include "hid.h"
#include "hid_internal.h"

static struct usb_setup_req hid_set_idle_descriptor = {
    USB_REQ_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_INTERFACE,
    USB_HID_SET_IDLE,
    0x0000,
    0x0000,
    0x0000,
};

enum {
  DEVICE_CONNECTED,
  DEVICE_IDLE,
  DEVICE_GET_REPORT_01,
  DEVICE_GET_REPORT_F2,
  DEVICE_GET_REPORT_F5,
  DEVICE_READY,
};

bool hid_dualshock3_check_device_desc(struct hid_info* hid_info,
                                      struct usb_info* usb_info,
                                      const struct usb_desc_device* desc) {
  if (desc->idVendor == 0x054c && desc->idProduct == 0x0268) {
    hid_info->type = HID_TYPE_PS3;
    usb_info->state = DEVICE_CONNECTED;
    return true;
  }
  return false;
}

void hid_dualshock3_initialize(struct hid_info* hid_info) {
  hid_info->button[0] = 23;   // □
  hid_info->button[1] = 22;   // ×
  hid_info->button[2] = 21;   // ◯
  hid_info->button[3] = 20;   // △
  hid_info->button[4] = 18;   // L1
  hid_info->button[5] = 19;   // R1
  hid_info->button[6] = 16;   // L2
  hid_info->button[7] = 17;   // R2
  hid_info->button[8] = 8;    // SELECT
  hid_info->button[9] = 11;   // START
  hid_info->button[10] = 9;   // L3
  hid_info->button[11] = 10;  // R3
  hid_info->button[12] = 24;  // PS

  hid_info->dpad[0] = 12;  // UP
  hid_info->dpad[1] = 14;  // DOWN
  hid_info->dpad[2] = 15;  // LEFT
  hid_info->dpad[3] = 13;  // RIGHT

  hid_info->axis[4] = 136;  // L2
  hid_info->axis_size[4] = 8;
  hid_info->axis_shift[4] = 0;
  hid_info->axis_sign[4] = false;
  hid_info->axis_polarity[4] = false;

  hid_info->axis[5] = 144;  // R2
  hid_info->axis_size[5] = 8;
  hid_info->axis_shift[5] = 0;
  hid_info->axis_sign[5] = false;
  hid_info->axis_polarity[5] = false;
}

bool hid_dualshock3_report(struct hid_info* hid_info,
                           struct usb_info* usb_info,
                           const uint8_t* data,
                           uint16_t size) {
  if (hid_info->type != HID_TYPE_PS3) {
    return false;
  }
  if (!size || usb_info->state != DEVICE_READY ||
      *data != hid_info->report_id) {
    // Just consume
    return true;
  }
  // Fallback to the default handler
  //   0: report id == 1
  //   1: report first byte
  //   2: b0; select, b1: L3, b2: R3, b3: start, b4: U, b5: R, b6: D, b7: L
  //   3: b0: L2, b1: R2, b2: L1, b3: R1, b4: △, b5: ◯, b6: ×, b7: □
  //   4: b0: PS
  //   6: Analog LX 8-bits (axis 0)
  //   7: Analog LY 8-bits (axis 1)
  //   8: Analog RX 8-bits (axis 2)
  //   9: Analog RY 8-bits (axis 3)
  //  18: Analog L2 8-bits
  //  19: Analog R2 8-bits
  //  20: Analog L1 8-bits
  //  21: Analog R1 8-bits
  return false;
}

void hid_dualshock3_poll(uint8_t hub,
                         struct hid_info* hid_info,
                         struct usb_info* usb_info) {
  if (usb_info->state == DEVICE_CONNECTED) {
    usb_host_setup(hub, &hid_set_idle_descriptor, 0);
    usb_info->state = DEVICE_IDLE;
  } else if (usb_info->state == DEVICE_IDLE) {
    usb_host_hid_get_report(hub, 3, hid_info->report_id, 64);
    usb_info->state = DEVICE_GET_REPORT_01;
  } else if (usb_info->state == DEVICE_GET_REPORT_01) {
    usb_host_hid_get_report(hub, 3, 0xf2, 64);
    usb_info->state = DEVICE_GET_REPORT_F2;
  } else if (usb_info->state == DEVICE_GET_REPORT_F2) {
    usb_host_hid_get_report(hub, 3, 0xf5, 64);
    usb_info->state = DEVICE_GET_REPORT_F5;
  } else if (usb_info->state == DEVICE_GET_REPORT_F5) {
    usb_info->state = DEVICE_READY;
  } else if (usb_info->state == DEVICE_READY) {
    usb_host_in(hub, usb_info->ep_in, 64);
  }
}