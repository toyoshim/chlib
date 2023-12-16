// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "usb/hid_device.h"

#include <stdint.h>

#include "ch559.h"
#include "serial.h"
#include "usb/usb.h"

// Descriptors are example to mimic a MD mini controller. Do not use them for
// production.
static const uint8_t device_desc[] = {
    0x12,  // size
    USB_DESC_DEVICE,
    0x00,
    0x02,  // USB version
    0x00,  // device class
    0x00,  // device subclass
    0x00,  // protocol
    0x40,  // max packet size
    0xa3,
    0x0c,  // vendor ID
    0x24,
    0x00,  // device ID
    0x06,
    0x02,  // device version
    0x00,  // manufacturer string index
    0x02,  // product string index
    0x00,  // serial number string index
    0x01,  // number of configurations
};

static const uint8_t config_desc[] = {
    0x09,                                // size
    USB_DESC_CONFIGURATION, 0x29, 0x00,  // total length
    0x01,                                // number of interfaces
    0x01,                                // index of this configuration
    0x00,                                // configuration name string index
    0x80,                                // attributes
    0x32,                                // 100mA

    // interface descriptor
    0x09,  // size
    USB_DESC_INTERFACE,
    0x00,  // index of this interface
    0x00,  // alternate setting for this interface
    0x02,  // number of endpoints
    0x03,  // interface class (HID)
    0x00,  // interface subclass
    0x00,  // interface protocol
    0x00,  // string index for interface

    // hid report descriptor
    0x09,                             // size
    USB_DESC_HID, 0x11, 0x01,         // BCD representation of HID verrsion
    0x00,                             // target country code
    0x01,                             // number of HID report
    USB_DESC_HID_REPORT, 0x65, 0x00,  // descriptor length

    // endpoint descriptor
    0x07,  // size
    USB_DESC_ENDPOINT,
    0x02,        // OUT endpoint number 2
    0x03,        // attribute: interrurpt endpoint
    0x40, 0x00,  // maximum packet size
    0x0a,        // poll interval 10ms

    // endpoint descriptor
    0x07,  // size
    USB_DESC_ENDPOINT,
    0x81,        // IN endpoint number 1
    0x03,        // attribute: interrurpt endpoint
    0x40, 0x00,  // maximum packet size
    0x0a,        // poll interval 10ms
};

static const uint8_t qualifier_desc[] = {
    0x0a,  // size
    USB_DESC_DEVICE_QUALIFIER,
    0x00,
    0x02,  // USB version
    0x00,  // device class
    0x00,  // device subclass
    0x00,  // protocol
    0x40,  // max packet size
    0x01,  // number of configurations
    0x00,  // reserved
};

static const uint8_t report_desc[] = {
    0x05, 0x01,        // usage page (desktop)
    0x09, 0x04,        // usage (joystick)
    0xa1, 0x01,        // collection (application)
    0xa1, 0x02,        // collection (logical)
    0x75, 0x08,        // report size (8)
    0x95, 0x05,        // report count (5)
    0x15, 0x00,        // logical minimum (0)
    0x26, 0xff, 0x00,  // logical maximum (255)
    0x35, 0x00,        // physical minimum (0)
    0x46, 0xff, 0x00,  // physical maximum (255)
    0x09, 0x30,        // usage (x)
    0x09, 0x30,        // usage (x)
    0x09, 0x30,        // usage (x)
    0x09, 0x30,        // usage (x)
    0x09, 0x31,        // usage (y)
    0x81, 0x02,        // input
    0x75, 0x04,        // report size (4)
    0x95, 0x01,        // report count (1)
    0x25, 0x07,        // logical maximum (7)
    0x46, 0x3b, 0x01,  // physical maximum (315)
    0x65, 0x14,        // unit (0x14)
    0x09, 0x00,        // usage (undefined)
    0x81, 0x42,        // input
    0x65, 0x00,        // unit (0)
    0x75, 0x01,        // report size (1)
    0x95, 0x0a,        // report count (10)
    0x25, 0x01,        // logical maximum (1)
    0x45, 0x01,        // physical maximum (1)
    0x05, 0x09,        // usage page (button)
    0x19, 0x01,        // usage minimum (1)
    0x29, 0x0a,        // usage maximum (10)
    0x81, 0x02,        // input
    0x06, 0x00, 0xff,  // usage page (vendor)
    0x75, 0x01,        // report size (1)
    0x95, 0x0a,        // report count (10)
    0x25, 0x01,        // logical maximum (1)
    0x45, 0x01,        // physical maximum (1)
    0x09, 0x01,        // usage page (vendor)
    0x81, 0x02,        // input
    0xc0,              // end collection
    0xa1, 0x02,        // collection (logical)
    0x75, 0x08,        // report size (8)
    0x95, 0x04,        // report count (4)
    0x46, 0xff, 0x00,  // physical maximum (255)
    0x26, 0xff, 0x00,  // logical maximum (255)
    0x09, 0x02,        // usage (mouse)
    0x91, 0x02,        // output
    0xc0,              // end collection
    0xc0,              // end collection
};

static const uint8_t string_0[] = {0x04, 0x03, 0x09,
                                   0x04};  // language descriptor

static const uint8_t string_2[] = {
    0x1c, 0x03, '6', 0, 'B', 0, ' ', 0, 'c', 0, 'o', 0, 'n', 0,
    't',  0,    'r', 0, 'o', 0, 'l', 0, 'l', 0, 'e', 0, 'r', 0,
};

static uint8_t get_descriptor_size(uint8_t type, uint8_t no) {
  switch (type) {
    case USB_DESC_DEVICE:
      return sizeof(device_desc);
    case USB_DESC_CONFIGURATION:
      return sizeof(config_desc);
    case USB_DESC_STRING:
      switch (no) {
        case 0:
          return sizeof(string_0);
        case 2:
          return sizeof(string_2);
      }
      break;
    case USB_DESC_DEVICE_QUALIFIER:
      return sizeof(qualifier_desc);
    case USB_DESC_HID_REPORT:
      return sizeof(report_desc);
  }
  return 0;
}

static const uint8_t* get_descriptor(uint8_t type, uint8_t no) {
  switch (type) {
    case USB_DESC_DEVICE:
      return device_desc;
    case USB_DESC_CONFIGURATION:
      return config_desc;
    case USB_DESC_STRING:
      switch (no) {
        case 0:
          return string_0;
        case 2:
          return string_2;
      }
      break;
    case USB_DESC_DEVICE_QUALIFIER:
      return qualifier_desc;
    case USB_DESC_HID_REPORT:
      return report_desc;
  }
  return 0;
}

static void get_report(uint8_t* buffer, uint8_t* len) {
  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = 0;  // x: 8-bits, 128 center
  buffer[4] = 0;  // y: 8-bits, 128 center
  buffer[5] = 0;  // buttons: X, A, B, Y, _, _, _, _
  buffer[6] = 0;  // buttons: _, _, Start, Select, _, _, C, Z
  buffer[7] = 0;
  *len = 8;
}

void main(void) {
  initialize();

  Serial.println("HID Example");

  struct hid_device device;
  device.get_descriptor_size = get_descriptor_size;
  device.get_descriptor = get_descriptor;
  device.get_report = get_report;
  // flags should be aligned with the configuration descriptor so that the
  // framework can setup DMA configurations correctly.
  hid_device_init(&device, UD_USE_EP1_IN | UD_USE_EP2_OUT);

  for (;;) {
    if (usb_device_is_ready_to_send(1)) {
      uint8_t buffer[8];
      uint8_t len = 8;
      get_report(buffer, &len);
      usb_device_send(1, buffer, len);
    }
  }
}