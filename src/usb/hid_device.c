// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hid_device.h"

static struct usb_device usb_device;
static struct hid_device* hid_device = 0;

static bool setup(const struct usb_setup_req* req,
                  uint8_t* buffer,
                  uint8_t* len) {
  if ((req->bRequestType & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_CLASS) {
    switch (req->bRequest) {
      case USB_HID_GET_REPORT: {
        hid_device->get_report(buffer, len);
        return true;
      }
      case USB_HID_SET_IDLE:
        // TODO: notify clients to ignore interrupt requests.
        *len = 0;
        return true;
      default:
        break;
    }
  }
  return false;
}

void hid_device_init(struct hid_device* device, uint8_t usb_flags) {
  hid_device = device;

  usb_device.get_descriptor_size = hid_device->get_descriptor_size;
  usb_device.get_descriptor = hid_device->get_descriptor;
  usb_device.setup = setup;
  usb_device.ep_out = 0;

  usb_device_init(&usb_device, usb_flags);
}