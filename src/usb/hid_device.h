// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __hid_device_h__
#define __hid_device_h__

#include <stdint.h>

#include "usb_device.h"

struct hid_device {
  uint8_t (*get_descriptor_size)(uint8_t type, uint8_t no);
  const uint8_t* (*get_descriptor)(uint8_t type, uint8_t no);
  void (*get_report)(uint8_t* buffer, uint8_t* len);
};

void hid_device_init(struct hid_device* device, uint8_t usb_flags);

#endif  // __usb_device_h__