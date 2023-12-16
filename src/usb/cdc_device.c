// Copyright 2023 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "cdc_device.h"
#include <stdint.h>

#include "../serial.h"
#include "usb.h"
#include "usb_device.h"

static struct usb_desc_device desc_device = {
    sizeof(struct usb_desc_device),  // bLength
    USB_DESC_DEVICE,                 // bDescriptorType
    0x0200,                          // bcdUSB
    USB_CLASS_CDC,                   // bDeviceClass
    0x00,                            // bDeviceSubClass
    0x00,                            // bDeviceProtocol
    64,                              // bMaxPacketSize0
    0,                               // idVendor
    0,                               // idProduct
    0,                               // bcdDevice
    1,                               // iManufacturer
    2,                               // iProduct
    3,                               // iSerialNumber
    1,                               // bNumConfigurations
};

static struct {
  struct usb_desc_configuration desc_config;
  struct usb_desc_interface desc_control_interface;
  struct {
    uint8_t bFunctionLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint16_t bcdCDC;
  } desc_cdc_header_functional;
  struct {
    uint8_t bFunctionLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bmCapabilities;
    uint8_t bDataInterface;
  } desc_cdc_call_functional;
  struct {
    uint8_t bFunctionLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bmCapabilities;
  } desc_cdc_acm_functional;
  struct {
    uint8_t bFunctionLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bMasterInterface;
    uint8_t bSlaveInterface0;
  } desc_cdc_union_functional;
  struct usb_desc_endpoint desc_ep1;
  struct usb_desc_interface desc_data_interface;
  struct usb_desc_endpoint desc_ep2;
  struct usb_desc_endpoint desc_ep3;
} desc_config_etc = {
    {
        sizeof(struct usb_desc_configuration),  // bLength
        USB_DESC_CONFIGURATION,                 // bDescriptorType
        sizeof(desc_config_etc),                // wTotalLength
        2,                                      // bNumInterfaces
        1,                                      // bConfigurationValue
        0,                                      // iConfiguration
        0x80,                                   // bmAttributes
        50,                                     // bMaxPower (100mA)
    },
    {
        sizeof(struct usb_desc_interface),  // bLength
        USB_DESC_INTERFACE,                 // bDescriptorType
        0,                                  // bInterfaceNumber
        0,                                  // bAlternateSetting
        1,                                  // bNumEndpoints
        USB_CLASS_CDC,                      // bInterfaceClass
        USB_CDC_SUBCLASS_ACM,               // bInterfaceSubClass
        0,                                  // bInterfaceProtocol
        0,                                  // iInterface
    },
    {
        sizeof(desc_config_etc.desc_cdc_header_functional),
        USB_DESC_CS_INTERFACE,
        USB_DESC_SUB_CS_HEADER,
        0x0110,
    },
    {
        sizeof(desc_config_etc.desc_cdc_call_functional),
        USB_DESC_CS_INTERFACE,
        USB_DESC_SUB_CS_CALL,
        0x00,
        0,
    },
    {
        sizeof(desc_config_etc.desc_cdc_acm_functional), USB_DESC_CS_INTERFACE,
        USB_DESC_SUB_CS_ACM,
        0x02,  // bmCapabilities: line coding
    },
    {
        sizeof(desc_config_etc.desc_cdc_union_functional),
        USB_DESC_CS_INTERFACE,
        USB_DESC_SUB_CS_UNION,
        0,
        1,
    },
    {
        sizeof(struct usb_desc_endpoint),  // bLength
        USB_DESC_ENDPOINT,                 // bDescriptorType
        0x81,                              // bEndpointAddress
        0x03,                              // bmAttributes
        64,                                // wMaxPacketSize
        1,                                 // bInterval
    },
    {
        sizeof(struct usb_desc_interface),  // bLength
        USB_DESC_INTERFACE,                 // bDescriptorType
        1,                                  // bInterfaceNumber
        0,                                  // bAlternateSetting
        2,                                  // bNumEndpoints
        USB_CLASS_CDC_DATA,                 // bInterfaceClass
        0,                                  // bInterfaceSubClass
        0,                                  // bInterfaceProtocol
        0,                                  // iInterface
    },
    {
        sizeof(struct usb_desc_endpoint),  // bLength
        USB_DESC_ENDPOINT,                 // bDescriptorType
        0x02,                              // bEndpointAddress
        0x02,                              // bmAttributes
        64,                                // wMaxPacketSize
        0,                                 // bInterval
    },
    {
        sizeof(struct usb_desc_endpoint),  // bLength
        USB_DESC_ENDPOINT,                 // bDescriptorType
        0x83,                              // bEndpointAddress
        0x02,                              // bmAttributes
        64,                                // wMaxPacketSize
        0,                                 // bInterval
    },
};

static struct usb_desc_qualifier usb_desc_qualifier = {
    sizeof(struct usb_desc_qualifier),  // bLength
    USB_DESC_DEVICE_QUALIFIER,          // bDescriptorType
    0x0110,                             // bcdUSB
    USB_CLASS_CDC,                      // bDeviceClass
    0x00,                               // bDeviceSubClass
    0x00,                               // bDeviceProtocol
    64,                                 // bMaxPacketSize0
    1,                                  // bNumConfigurations
    0,                                  // bReserved
};

static struct usb_device usb_device;
static struct cdc_device* cdc_device = 0;
static const uint8_t kString00Language[] = {4, 3, 0x09, 0x04};
static char descriptor_buffer[64];
static uint8_t last_request = 0;
static uint32_t baud_rate = 115200;
static uint8_t char_format = 0;  // 1 stop bit
static uint8_t parity_type = 0;  // none
static uint8_t data_bits = 8;    // data bits

static uint8_t get_string_descriptor_size(uint8_t no) {
  switch (no) {
    case 0:
      return sizeof(kString00Language);
    case 1:
    case 2:
    case 3:
      return cdc_device->get_string_length(no) * 2 + 2;
  }
  return 0;
}

static const uint8_t* get_string_descriptor(uint8_t no) {
  uint8_t length = cdc_device->get_string_length(no);
  const char* string = cdc_device->get_string(no);
  switch (no) {
    case 0:
      return kString00Language;
    case 1:
    case 2:
    case 3:
      descriptor_buffer[0] = length * 2 + 2;
      descriptor_buffer[1] = USB_DESC_STRING;
      for (uint8_t i = 0; i < length; ++i) {
        descriptor_buffer[2 + i * 2] = string[i];
        descriptor_buffer[2 + i * 2 + 1] = 0;
      }
      return descriptor_buffer;
  }
  return 0;
}

static uint8_t get_descriptor_size(uint8_t type, uint8_t no) {
  switch (type) {
    case USB_DESC_DEVICE:
      return sizeof(struct usb_desc_device);
    case USB_DESC_CONFIGURATION:
      return sizeof(desc_config_etc);
    case USB_DESC_STRING:
      return get_string_descriptor_size(no);
    case USB_DESC_DEVICE_QUALIFIER:
      return sizeof(struct usb_desc_qualifier);
  }
  return 0;
}

static const uint8_t* get_descriptor(uint8_t type, uint8_t no) {
  switch (type) {
    case USB_DESC_DEVICE:
      return (const uint8_t*)&desc_device;
    case USB_DESC_CONFIGURATION:
      return (const uint8_t*)&desc_config_etc;
    case USB_DESC_STRING:
      return get_string_descriptor(no);
    case USB_DESC_DEVICE_QUALIFIER:
      return (const uint8_t*)&usb_desc_qualifier;
  }
  return 0;
}

static bool setup(const struct usb_setup_req* req,
                  uint8_t* buffer,
                  uint8_t* len) {
  last_request = req->bRequest;
  if (req->bRequestType ==
      (USB_REQ_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_INTERFACE)) {
    switch (req->bRequest) {
      case USB_CDC_GET_LINE_CODING:
        buffer[0] = baud_rate & 0xff;
        buffer[1] = (baud_rate >> 8) & 0xff;
        buffer[2] = (baud_rate >> 16) & 0xff;
        buffer[3] = (baud_rate >> 24) & 0xff;
        buffer[4] = char_format;
        buffer[5] = parity_type;
        buffer[6] = data_bits;
        *len = 7;
        return true;
    }
  } else if (req->bRequestType ==
             (USB_REQ_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_INTERFACE)) {
    switch (req->bRequest) {
      case USB_CDC_SET_LINE_CODING:
      case USB_CDC_SET_CONTROL_LINE_STATE:
        *len = 0;
        return true;
    }
  }
  return false;
}

static bool ep_out(uint8_t no, const uint8_t* buffer, uint8_t len) {
  if (no == 0) {
    switch (last_request) {
      case USB_CDC_SET_LINE_CODING:
        // TODO: Notify via cdc_device callback.
        if (len == 7) {
          baud_rate = buffer[3];
          baud_rate <<= 8;
          baud_rate |= buffer[2];
          baud_rate <<= 8;
          baud_rate |= buffer[1];
          baud_rate <<= 8;
          baud_rate |= buffer[0];
          char_format = buffer[4];
          parity_type = buffer[5];
          data_bits = buffer[6];
        }
        last_request = 0;
        return true;
      case USB_CDC_GET_LINE_CODING:
        last_request = 0;
        return true;
      case USB_CDC_SET_CONTROL_LINE_STATE:
        last_request = 0;
        return true;
    }
  } else if (no == 2) {
    cdc_device->recv(buffer, len);
    return true;
  }
  return false;
}

void cdc_device_init(struct cdc_device* device) {
  cdc_device = device;

  desc_device.idVendor = device->id_vendor;
  desc_device.idProduct = device->id_product;
  desc_device.bcdDevice = device->bcd_device;

  usb_device.get_descriptor_size = get_descriptor_size;
  usb_device.get_descriptor = get_descriptor;
  usb_device.setup = setup;
  usb_device.ep_out = ep_out;

  usb_device_init(&usb_device, UD_USE_EP1_IN | UD_USE_EP2_OUT | UD_USE_EP3_IN);
}

void cdc_device_send(const uint8_t* buffer, uint8_t len) {
  while (!usb_device_is_ready_to_send(3))
    ;
  usb_device_send(3, buffer, len);
}