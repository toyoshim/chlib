// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __cdc_device_h__
#define __cdc_device_h__

#include <stdint.h>
#include "usb_device.h"

enum {
  CDC_STRING_MANUFACTURER = 1,
  CDC_STRING_PRODUCT = 2,
  CDC_STRING_SERIAL = 3,
};

struct cdc_device {
  uint16_t id_vendor;
  uint16_t id_product;
  uint16_t bcd_device;

  uint8_t (*get_string_length)(uint8_t no);
  const char*(get_string)(uint8_t no);
  void (*send)(uint8_t* buffer, uint8_t* len);
  void (*recv)(const uint8_t* buffer, uint8_t len);
};

void cdc_device_init(struct cdc_device* device);

#endif  // __cdc_device_h__