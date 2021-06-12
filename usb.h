// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __usb_h__
#define __usb_h__

#include <stdint.h>

struct USB_SETUP_REQ {
  uint8_t bRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
};

enum {
  // bRequestType
  USB_REQ_TYP_MASK = 0x60,
  USB_REQ_TYP_STANDARD = 0x00,

  // bRequest
  USB_GET_STATUS = 0x00,
  USB_CLEAR_FEATURE = 0x01,
  USB_SET_FEATURE = 0x03,
  USB_SET_ADDRESS = 0x05,
  USB_GET_DESCRIPTOR = 0x06,
  USB_SET_DESCRIPTOR = 0x07,
  USB_GET_CONFIGURATION = 0x08,
  USB_SET_CONFIGURATION = 0x09,
  USB_GET_INTERFACE = 0x0a,
  USB_SET_INTERFACE = 0x0b,
  USB_SYNCH_FRAME = 0x0c,
};

#endif  // __usb_h__