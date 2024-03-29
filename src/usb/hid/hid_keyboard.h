// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __hid_keyboard_h__
#define __hid_keyboard_h__

#include <stdbool.h>

struct hid_info;
struct usb_desc_device;
struct usb_desc_interface;

bool hid_keyboard_check_device_desc(struct hid_info* hid_info,
                                    const struct usb_desc_device* desc);

bool hid_keyboard_check_interface_desc(struct hid_info* hid_info,
                                       const struct usb_desc_interface* desc);

bool hid_keyboard_initialize(struct hid_info* hid_info);

#endif  // __hid_keyboard_h__