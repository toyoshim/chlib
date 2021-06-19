// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __usb_h__
#define __usb_h__

#include <stdint.h>

struct usb_setup_req {
  uint8_t bRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
};

struct usb_desc_head {
  uint8_t bLength;
  uint8_t bDescriptorType;
};

struct usb_desc_device {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize0;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t iManufacturer;
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumConfigurations;
};

struct usb_desc_configuration {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
};

struct usb_desc_interface {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
};

struct usb_desc_hid {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdHID;
  uint8_t bCountryCode;
  uint8_t bNumDescriptors;
  uint8_t bReportDescriptorType;
  uint16_t wDescriptorLength;
};

struct usb_desc_endpoint {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
};

enum {
  // bRequestType
  USB_REQ_DIR_MASK = 0x80,
  USB_REQ_DIR_OUT = 0x00,
  USB_REQ_DIR_IN = 0x80,

  USB_REQ_TYPE_MASK = 0x60,
  USB_REQ_TYPE_STANDARD = 0x00,
  USB_REQ_TYPE_CLASS = 0x20,
  USB_REQ_TYPE_VENDOR = 0x40,

  USB_REQ_RECPT_MASK = 0x1f,
  USB_REQ_RECPT_DEVICE = 0x00,
  USB_REQ_RECPT_INTERFACE = 0x01,
  USB_REQ_RECPT_ENDPOINT = 0x02,

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

  // descriptor
  USB_DESC_DEVICE = 0x01,
  USB_DESC_CONFIGURATION = 0x02,
  USB_DESC_STRING = 0x03,
  USB_DESC_INTERFACE = 0x04,
  USB_DESC_ENDPOINT = 0x05,
  USB_DESC_HID = 0x21,
  USB_DESC_HID_REPORT = 0x22,

  // pid
  USB_PID_OUT = 0x01,
  USB_PID_ACK = 0x02,
  USB_PID_DATA0 = 0x03,
  USB_PID_IN = 0x09,
  USB_PID_NAK = 0x0a,
  USB_PID_DATA1 = 0x0b,
  USB_PID_SETUP = 0x0d,
  USB_PID_STALL = 0x0e,

  // class
  USB_CLASS_AUDIO = 0x01,
  USB_CLASS_CDC = 0x02,
  USB_CLASS_HID = 0x03,
};

#endif  // __usb_h__