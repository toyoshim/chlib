// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "hid_mouse.h"

#include "hid.h"
#include "usb.h"

static bool check(uint8_t any_class,
                  uint8_t any_subclass,
                  uint8_t any_protocol) {
  return any_class == USB_CLASS_HID && any_subclass == USB_HID_SUBCLASS_BOOT &&
         any_protocol == USB_HID_PROTOCOL_MOUSE;
}

bool hid_mouse_check_device_desc(struct hub_info* hub_info,
                                 const struct usb_desc_device* desc) {
  if (check(desc->bDeviceClass, desc->bDeviceSubClass, desc->bDeviceProtocol)) {
    hub_info->type = HID_TYPE_MOUSE;
    return true;
  }
  return false;
}

bool hid_mouse_check_interface_desc(struct hub_info* hub_info,
                                    const struct usb_desc_interface* desc) {
  if (check(desc->bInterfaceClass, desc->bInterfaceSubClass,
            desc->bInterfaceProtocol)) {
    hub_info->type = HID_TYPE_MOUSE;
    return true;
  }
  return false;
}