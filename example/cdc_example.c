// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "usb/cdc_device.h"

#include "ch559.h"
#include "serial.h"

static const char kString01Manufacturer[] = "Mellow PCB - mellow.twintail.org";
static const char kString02Product[] = "CDC Example";
static const char kString03SerialNumber[] = "EOS:";

static int offset = 6;
static char kHello[] = "Hello";

static uint8_t get_string_length(uint8_t no) {
  switch (no) {
    case 1:
      return sizeof(kString01Manufacturer) - 1;
    case 2:
      return sizeof(kString02Product) - 1;
    case 3:
      return sizeof(kString03SerialNumber) - 1;
  }
  return 0;
}
static const char* get_string(uint8_t no) {
  switch (no) {
    case 1:
      return kString01Manufacturer;
    case 2:
      return kString02Product;
    case 3:
      return kString03SerialNumber;
  }
  return 0;
}

void recv(const uint8_t* buffer, uint8_t len) {
  // Echo back
  Serial.printf("recv: %x, %d\n", *buffer, len);
  cdc_device_send(buffer, len);
}

void main(void) {
  initialize();

  Serial.println("CDC Example");

  struct cdc_device device;
  device.id_vendor = 0x0483;
  device.id_product = 0x16c0;
  device.bcd_device = 0x0100;
  device.get_string_length = get_string_length;
  device.get_string = get_string;
  device.recv = recv;

  cdc_device_init(&device);

  for (;;)
    ;
}