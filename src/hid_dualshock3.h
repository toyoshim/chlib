// Copyright 2022 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __hid_dualshock3_h__
#define __hid_dualshock3_h__

#include <stdbool.h>
#include <stdint.h>

struct hub_info;
struct usb_desc_device;
struct usb_info;

bool hid_dualshock3_check_device_desc(struct hub_info* hub_info,
                                      struct usb_info* usb_info,
                                      const struct usb_desc_device* desc);

void hid_dualshock3_initialize(struct hub_info* hub_info);

bool hid_dualshock3_report(struct hub_info* hub_info,
                           struct usb_info* usb_info,
                           const uint8_t* data,
                           uint16_t size);

void hid_dualshock3_poll(uint8_t hub,
                         struct hub_info* hub_info,
                         struct usb_info* usb_info);

#endif  // __hid_dualshock3_h__