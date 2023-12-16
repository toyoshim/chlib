// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __usb_device_h__
#define __usb_device_h__

#include <stdbool.h>
#include <stdint.h>

#include "../interrupt.h"
#include "usb.h"

extern void usb_int(void) __interrupt(INT_NO_USB) __using(1);

struct usb_device {
  uint8_t (*get_descriptor_size)(uint8_t type, uint8_t no);
  const uint8_t* (*get_descriptor)(uint8_t type, uint8_t no);
  bool (*setup)(const struct usb_setup_req* req, uint8_t* buffer, uint8_t* len);
  bool (*ep_out)(uint8_t no, const uint8_t* buffer, uint8_t len);
};

enum {
  // flags
  UD_USE_EP1_OUT = 1 << 0,
  UD_USE_EP2_OUT = 1 << 1,
  UD_USE_EP3_OUT = 1 << 2,
  UD_USE_EP1_IN = 1 << 4,
  UD_USE_EP2_IN = 1 << 5,
  UD_USE_EP3_IN = 1 << 6,

  // states
  UD_STATE_IDLE = 0,
  UD_STATE_SETUP = 1,
  UD_STATE_READY = 2,
};

void usb_device_init(struct usb_device* device, uint8_t flags);
uint8_t usb_device_state(void);
bool usb_device_is_ready_to_send(uint8_t ep);
void usb_device_send(uint8_t ep, const uint8_t* buffer, uint8_t len);

#endif  // __usb_device_h__