// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "hid_mouse.h"

#include "hid.h"
#include "hid_internal.h"
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
                                    struct usb_info* usb_info,
                                    const struct usb_desc_interface* desc) {
  if (check(desc->bInterfaceClass, desc->bInterfaceSubClass,
            desc->bInterfaceProtocol)) {
    hub_info->type = HID_TYPE_MOUSE;
    return true;
  }
  if (usb_info->vid == 0xd209 && usb_info->pid == 0x1601 &&
      desc->bInterfaceNumber == 2) {
    // AimTrak needs to use multiple interfaces.
    // As a basic support, let's use the 3rd interface to obtain the point
    // address, and the trigger click inside or outside the screen.
    // Red buttons in left and right cannot be accessed here, as they need to
    // tweak the 2nd interface with bInterfaceNumber == 1.
    hub_info->type = HID_TYPE_MOUSE;
    return true;
  }
  return false;
}