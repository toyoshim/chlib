// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "usb_device.h"
#include <stdint.h>

#include "../ch559.h"
#include "../io.h"
#include "../serial.h"

#define NOTREACHED(e) halt(e)

static struct usb_device* usb_device = 0;
static uint8_t usb_device_flags = 0;
static uint8_t state = UD_STATE_IDLE;

static const uint8_t ep0_size = 64;
static uint8_t _ep0_buffer[64 + 1];  // EP0 buffer size 64
static uint8_t* ep0_buffer = _ep0_buffer;
static uint8_t _ep1_buffer[64 + 1];  // EP1 buffer size 64
static uint8_t* ep1_buffer = _ep1_buffer;
static uint8_t _ep2_buffer[64 + 1];  // EP2 buffer size 64
static uint8_t* ep2_buffer = _ep2_buffer;
static uint8_t _ep3_buffer[64 + 1];  // EP3 buffer size 64
static uint8_t* ep3_buffer = _ep3_buffer;

static struct usb_setup_req last_setup_req;
static const uint8_t* sending_data_ptr = 0;
static uint8_t sending_data_len = 0;

static void halt(const char* message) {
#if 0
  message;
#else
  Serial.printf("HALT: %s\n", message);
  Serial.print("type: ");
  Serial.printc(last_setup_req.bRequestType, HEX);
  Serial.println("");
  Serial.print("req: ");
  Serial.printc(last_setup_req.bRequest, HEX);
  Serial.println("");
  Serial.print("value: ");
  Serial.printc(last_setup_req.wValue >> 8, HEX);
  Serial.printc(last_setup_req.wValue & 0xff, HEX);
  Serial.println("");
  Serial.print("index: ");
  Serial.printc(last_setup_req.wIndex >> 8, HEX);
  Serial.printc(last_setup_req.wIndex & 0xff, HEX);
  Serial.println("");
  Serial.print("length: ");
  Serial.printc(last_setup_req.wLength >> 8, HEX);
  Serial.printc(last_setup_req.wLength & 0xff, HEX);
  Serial.println("");
  for (;;)
    ;
#endif
}

static uint8_t* get_buffer(uint8_t ep) {
  switch (ep) {
    case 0:
      return ep0_buffer;
    case 1:
      return ep1_buffer;
    case 2:
      return ep2_buffer;
    case 3:
      return ep3_buffer;
    default:
      return 0;
  }
}

static void bus_reset(void) {
  // ACK for SETUP and OUT, NAK for IN.
  UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
  if (usb_device_flags & (UD_USE_EP1_OUT | UD_USE_EP1_IN)) {
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
  }
  if (usb_device_flags & (UD_USE_EP2_OUT | UD_USE_EP2_IN)) {
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
  }
  if (usb_device_flags & (UD_USE_EP3_OUT | UD_USE_EP3_IN)) {
    UEP3_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
  }
  USB_DEV_AD = 0x00;
}

static void stall(void) {
  UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
}

static void ep0_send(uint8_t len, const uint8_t* data) {
  uint8_t transfer_len = (len <= ep0_size) ? len : ep0_size;
  if (data) {
    for (uint8_t i = 0; i < transfer_len; ++i) {
      ep0_buffer[i] = data[i];
    }
  }
  UEP0_T_LEN = transfer_len;
  UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
  sending_data_ptr = &data[transfer_len];
  sending_data_len = len - transfer_len;
}

static void ep0_cont(void) {
  uint8_t transfer_len =
      (sending_data_len <= ep0_size) ? sending_data_len : ep0_size;
  for (uint8_t i = 0; i < transfer_len; ++i) {
    ep0_buffer[i] = sending_data_ptr[i];
  }
  UEP0_T_LEN = transfer_len;
  UEP0_CTRL ^= (bUEP_R_TOG | bUEP_T_TOG);
  sending_data_ptr += transfer_len;
  sending_data_len -= transfer_len;
}

static void get_descriptor(void) {
  uint8_t type = last_setup_req.wValue >> 8;
  uint8_t no = last_setup_req.wValue & 0xff;
  uint8_t size = usb_device->get_descriptor_size(type, no);
  if (0 == size) {
    NOTREACHED("unknown descriptor");
    return;
  }
  if (size > last_setup_req.wLength) {
    size = last_setup_req.wLength;
  }
  ep0_send(size, usb_device->get_descriptor(type, no));
}

static void setup(void) {
  if (USB_RX_LEN != sizeof(struct usb_setup_req)) {
    NOTREACHED("unexpected request size");
    return;
  }
  struct usb_setup_req* req = (struct usb_setup_req*)ep0_buffer;
  last_setup_req = *req;
  if ((req->bRequestType & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD) {
    switch (req->bRequest) {
      case USB_GET_STATUS:
        ep0_send(2, "\0\0");
        return;
      case USB_CLEAR_FEATURE:
        ep0_send(0, 0);
        return;
      case USB_SET_FEATURE:
        ep0_send(0, 0);
        return;
      case USB_SET_ADDRESS:
        ep0_send(0, 0);
        return;
      case USB_GET_DESCRIPTOR:
        get_descriptor();
        return;
      case USB_GET_CONFIGURATION:
        ep0_send(1, "\0");
        return;
      case USB_SET_CONFIGURATION:
        ep0_send(0, 0);
        state = UD_STATE_READY;
        return;
      default:
        break;
    }
  } else {
    uint8_t len = 0;
    if (usb_device->setup && usb_device->setup(req, ep0_buffer, &len)) {
      ep0_send(len, 0);
      return;
    }
  }
  NOTREACHED("setup");
}

void in(void) {
  if ((last_setup_req.bRequestType & USB_REQ_TYPE_MASK) ==
      USB_REQ_TYPE_STANDARD) {
    if (last_setup_req.bRequest == USB_SET_ADDRESS) {
      USB_DEV_AD = last_setup_req.wValue;
      UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
      return;
    }
  }
  if ((last_setup_req.bRequestType & USB_REQ_DIR_MASK) == USB_REQ_DIR_IN) {
    ep0_cont();
  } else {
    // Status Out
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
  }
}

void out(void) {
  if ((last_setup_req.bRequestType & USB_REQ_DIR_MASK) == USB_REQ_DIR_IN) {
    // Status Out
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    return;
  }
  if ((last_setup_req.bRequestType & USB_REQ_TYPE_MASK) ==
      USB_REQ_TYPE_STANDARD) {
    NOTREACHED("out0 standard");
  } else {
    bool result = false;
    if (usb_device->ep_out) {
      result = usb_device->ep_out(0, ep0_buffer, USB_RX_LEN);
    }
    if (!result) {
      NOTREACHED("out0");
    }
    UEP0_CTRL ^= bUEP_R_TOG;
  }
}

void ep_in(uint8_t ep) {
  if (ep == 1) {
    UEP1_T_LEN = 0;
    UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
  } else if (ep == 2) {
    UEP2_T_LEN = 0;
    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
  } else {
    UEP3_T_LEN = 0;
    UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
  }
}

void ep_out(uint8_t ep) {
  // Consider only single data transaction
  bool result = false;
  if (usb_device->ep_out) {
    result = usb_device->ep_out(ep, get_buffer(ep), USB_RX_LEN);
  }
  if (!result) {
    NOTREACHED("ep_out");
  }
  if (ep == 1) {
    UEP1_T_LEN = 0;
    UEP1_CTRL ^= bUEP_R_TOG;
  } else if (ep == 2) {
    UEP2_T_LEN = 0;
    UEP2_CTRL ^= bUEP_R_TOG;
  } else {
    UEP3_T_LEN = 0;
    UEP3_CTRL ^= bUEP_R_TOG;
  }
}

void usb_int(void) __interrupt(INT_NO_USB) __using(1) {
  if (UIF_TRANSFER) {
    uint8_t usb_int_st = USB_INT_ST;
    // For EP0
    switch (usb_int_st & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {
      case UIS_TOKEN_SETUP:
        setup();
        break;
      case UIS_TOKEN_IN:
        in();
        break;
      case UIS_TOKEN_OUT:
        out();
        break;
      default:
        // For other EP
        switch (usb_int_st & MASK_UIS_TOKEN) {
          case UIS_TOKEN_IN:
            ep_in(usb_int_st & MASK_UIS_ENDP);
            break;
          case UIS_TOKEN_OUT:
            ep_out(usb_int_st & MASK_UIS_ENDP);
            break;
          default:
            NOTREACHED("Unexpected token");
            break;
        }
        break;
    }
    UIF_TRANSFER = 0;
  } else if (UIF_BUS_RST) {
    bus_reset();
    UIF_TRANSFER = 0;
    UIF_BUS_RST = 0;
  } else {
    NOTREACHED("usb_int: unknown reason");
  }
}

void usb_device_init(struct usb_device* device, uint8_t flags) {
  usb_device = device;
  usb_device_flags = flags;

  // DMA addresses must be even
  if ((uint16_t)ep0_buffer & 1) {
    ep0_buffer++;
  }
  if ((uint16_t)ep1_buffer & 1) {
    ep1_buffer++;
  }
  if ((uint16_t)ep2_buffer & 1) {
    ep2_buffer++;
  }
  if ((uint16_t)ep3_buffer & 1) {
    ep3_buffer++;
  }
  IE_USB = 0;       // Disable USB interrupts
  USB_CTRL = 0x00;  // Device, full speed, disble, no pu--up, no pause, no DMA
  UEP4_1_MOD = 0;
  if (flags & UD_USE_EP1_OUT) {
    UEP4_1_MOD |= bUEP1_RX_EN;
  } else if (flags & UD_USE_EP1_IN) {
    UEP4_1_MOD |= bUEP1_TX_EN;
  }
  UEP2_3_MOD = 0;
  if (flags & UD_USE_EP2_OUT) {
    UEP2_3_MOD |= bUEP2_RX_EN;
  } else if (flags & UD_USE_EP2_IN) {
    UEP2_3_MOD |= bUEP2_TX_EN;
  }
  if (flags & UD_USE_EP3_OUT) {
    UEP2_3_MOD |= bUEP3_RX_EN;
  } else if (flags & UD_USE_EP3_IN) {
    UEP2_3_MOD |= bUEP3_TX_EN;
  }
  UEP0_DMA_H = (uint16_t)ep0_buffer >> 8;
  UEP0_DMA_L = (uint16_t)ep0_buffer & 0xff;
  if (flags & (UD_USE_EP1_OUT | UD_USE_EP1_IN)) {
    UEP1_DMA_H = (uint16_t)ep1_buffer >> 8;
    UEP1_DMA_L = (uint16_t)ep1_buffer & 0xff;
  }
  if (flags & (UD_USE_EP2_OUT | UD_USE_EP2_IN)) {
    UEP2_DMA_H = (uint16_t)ep2_buffer >> 8;
    UEP2_DMA_L = (uint16_t)ep2_buffer & 0xff;
  }
  if (flags & (UD_USE_EP3_OUT | UD_USE_EP3_IN)) {
    UEP3_DMA_H = (uint16_t)ep3_buffer >> 8;
    UEP3_DMA_L = (uint16_t)ep3_buffer & 0xff;
  }
  bus_reset();
  UDEV_CTRL = bUD_DP_PD_DIS | bUD_DM_PD_DIS;  // Release pull-downs for host
  USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;
  UDEV_CTRL |= bUD_PORT_EN;  // Enable USB port
  USB_INT_FG = 0xff;         // Clear interrupt flags
  USB_INT_EN = bUIE_TRANSFER | bUIE_BUS_RST;
  IE_USB = 1;  // Enable USB interrupts
  EA = 1;      // Enable interrupts
}

uint8_t usb_device_state(void) {
  return state;
}

bool usb_device_is_ready_to_send(uint8_t ep) {
  switch (ep) {
    case 0:
      return (UEP0_CTRL & MASK_UEP_T_RES) != UEP_T_RES_ACK;
    case 1:
      return (UEP1_CTRL & MASK_UEP_T_RES) != UEP_T_RES_ACK;
    case 2:
      return (UEP2_CTRL & MASK_UEP_T_RES) != UEP_T_RES_ACK;
    case 3:
      return (UEP3_CTRL & MASK_UEP_T_RES) != UEP_T_RES_ACK;
  }
  return false;
}

void usb_device_send(uint8_t ep, const uint8_t* data, uint8_t len) {
  uint8_t* buffer = get_buffer(ep);
  for (uint8_t i = 0; i < len; ++i) {
    buffer[i] = data[i];
  }
  switch (ep) {
    case 0:
      UEP0_T_LEN = len;
      UEP0_CTRL = UEP0_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
      break;
    case 1:
      UEP1_T_LEN = len;
      UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
      break;
    case 2:
      UEP2_T_LEN = len;
      UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
      break;
    case 3:
      UEP3_T_LEN = len;
      UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
      break;
  }
}