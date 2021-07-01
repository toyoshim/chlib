// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __usb_host_h__
#define __usb_host_h__

#include <stdbool.h>
#include <stdint.h>

// Should be included from the main source file to set up interrupt handler.
#include "timer3.h"

enum {
  USE_HUB0 = 1 << 0,
  USE_HUB1 = 1 << 1,
};

struct usb_host {
  uint8_t flags;
  void (*disconnected)(uint8_t hub);
  void (*check_device_desc)(uint8_t hub, const uint8_t* desc);
  void (*check_configuration_desc)(uint8_t hub, const uint8_t* desc);
  void (*check_hid_report_desc)(uint8_t hub, const uint8_t* desc);
  void (*in)(uint8_t hub, const uint8_t* data);
};

void usb_host_init(struct usb_host* host);
void usb_host_poll();
bool usb_host_ready(uint8_t hub);
bool usb_host_idle();
bool usb_host_in(uint8_t hub, uint8_t ep, uint8_t size);

#endif  // __usb_host_h__