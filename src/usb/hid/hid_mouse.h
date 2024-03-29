// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __hid_mouse_h__
#define __hid_mouse_h__

#include <stdbool.h>

struct hid_info;
struct usb_info;
struct usb_desc_device;
struct usb_desc_interface;

bool hid_mouse_check_device_desc(struct hid_info* hid_info,
                                 const struct usb_desc_device* desc);

bool hid_mouse_check_interface_desc(struct hid_info* hid_info,
                                    struct usb_info* usb_info,
                                    const struct usb_desc_interface* desc);

#endif  // __hid_mouse_h__