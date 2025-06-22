// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __hid_internal_h__
#define __hid_internal_h__

#include <stdint.h>

struct usb_info {
  uint8_t class;
  uint8_t subclass;
  uint8_t protocol;
  uint16_t vid;
  uint16_t pid;
  uint16_t device;
  uint16_t ep_max_packet_size;
  uint8_t ep_out;
  uint8_t ep_in;
  uint8_t state;
  uint8_t cmd_count;
  uint8_t interface;
  uint16_t tick;
  uint16_t wait;
  uint16_t get_report_value;
  uint8_t get_report_length;
};

#endif  // __hid_internal_h__