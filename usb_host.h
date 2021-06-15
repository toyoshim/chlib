// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __usb_host_h__
#define __usb_host_h__

#include <stdint.h>

enum {
  USE_HUB0 = 1 << 0,
  USE_HUB1 = 1 << 1,
};

struct usb_host {
  uint8_t flags;
  void (*check_device_desc)(uint8_t hub, const uint8_t* desc);
};

void usb_host_init(struct usb_host* host);
void usb_host_poll();

#endif  // __usb_host_h__