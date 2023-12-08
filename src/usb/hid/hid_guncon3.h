// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __hid_guncon3_h__
#define __hid_guncon3_h__

#include <stdbool.h>
#include <stdint.h>

struct hid_info;
struct usb_info;
struct usb_desc_device;
struct usb_desc_interface;

bool hid_guncon3_check_device_desc(struct hid_info* hid_info,
                                   struct usb_info* usb_info,
                                   const struct usb_desc_device* desc);

bool hid_guncon3_check_interface_desc(struct hid_info* hid_info,
                                      struct usb_info* usb_info);

bool hid_guncon3_initialize(struct hid_info* hid_info,
                            struct usb_info* usb_info);

bool hid_guncon3_report(struct hid_info* hid_info,
                        struct usb_info* usb_info,
                        uint8_t* data,
                        uint16_t size);

void hid_guncon3_poll(uint8_t hub, struct usb_info* usb_info);

#endif  // __hid_guncon3_h__