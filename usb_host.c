// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "usb_host.h"

#include "ch559.h"
#include "io.h"
#include "timer3.h"
#include "usb.h"

enum {
  STATE_IDLE,

  STATE_CONNECT,
  STATE_RESET,
  STATE_ENABLE,
  STATE_GET_DEVICE_DESC,
  STATE_GET_DEVICE_DESC_RECV,
  STATE_GET_CONFIGURATION_DESC,
  STATE_GET_CONFIGURATION_DESC_RECV,
  STATE_GET_FULL_CONFIGURATION_DESC,
  STATE_GET_FULL_CONFIGURATION_DESC_RECV,
  STATE_SET_CONFIGURATION,
  STATE_HID_SETUP,
  STATE_GET_HID_REPORT_DESC,
  STATE_GET_HID_REPORT_DESC_RECV,

  STATE_DONE,
  STATE_READY,

  STATE_IN_RECV,

  STATE_DELAY_US,
  STATE_DELAY_MS,
  STATE_TRANSACTION,
  STATE_TRANSACTION_IN,
  STATE_TRANSACTION_ACK,
  STATE_TRANSACTION_CONT,
  STATE_TRANSACTION_RETRY,
};

static const struct usb_setup_req get_device_descriptor = {
  USB_REQ_DIR_IN,
  USB_GET_DESCRIPTOR,
  USB_DESC_DEVICE << 8,
  0x00,
  0x12,
};
static struct usb_setup_req get_configuration_descriptor = {
  USB_REQ_DIR_IN,
  USB_GET_DESCRIPTOR,
  USB_DESC_CONFIGURATION << 8,
  0x00,
  0x09,  // requesting descriptor size: can be modified
};
static struct usb_setup_req set_configuration_descriptor = {
  USB_REQ_DIR_OUT,
  USB_SET_CONFIGURATION,
  0x00,  // configuration id: can be modified
  0x00,
  0x00,
};
static struct usb_setup_req get_hid_report_descriptor = {
  USB_REQ_DIR_IN | USB_REQ_RECPT_INTERFACE,
  USB_GET_DESCRIPTOR,
  USB_DESC_HID_REPORT << 8,
  0x00,
  0x00,  // requesting descriptor size: can be modified
};

static struct usb_host* usb_host = 0;

static uint8_t _rx_buffer[64 + 1];
static uint8_t* rx_buffer = _rx_buffer;
static uint8_t in_buffer[1024];
static uint8_t _tx_buffer[64 + 1];
static uint8_t* tx_buffer = _tx_buffer;

static int8_t transaction_lock = -1;
static uint8_t* transaction_buffer = 0;
static uint16_t transaction_size = 0;
static uint8_t transaction_recv_state = STATE_IDLE;
static uint8_t transaction_ep_pid = 0;

static uint8_t state[2] = { STATE_IDLE, STATE_IDLE };
static uint16_t delay_begin[2] = { 0, 0 };
static uint16_t delay_end[2] = { 0, 0 };
static uint8_t delay_next_state[2] = { 0, 0 };
static bool resetting[2] = { false, false };
static bool is_hid[2] = { false, false };

static void dump(const char* message, const uint8_t* buffer, uint16_t size) {
  Serial.print("=== ");
  Serial.print(message);
  Serial.println(" ===");
  for (uint16_t i = 0; i < size; ++i) {
    Serial.printc(buffer[i], HEX);
    if (i % 8 == 7)
      Serial.println("");
    else
      Serial.print(", ");
  }
  Serial.println("\r\n======");
}

static void halt(const char* message) {
  Serial.println(message);
  for (;;);
}

static void delay_us(uint8_t hub, uint16_t delay_us, uint8_t next_state) {
  delay_begin[hub] = timer3_tick_raw();
  delay_end[hub] = delay_begin[hub] + timer3_tick_from_usec(delay_us);
  delay_next_state[hub] = next_state;
  state[hub] = STATE_DELAY_US;
}

static void delay_ms(uint8_t hub, uint16_t delay_ms, uint8_t next_state) {
  delay_begin[hub] = timer3_tick_msec();
  delay_end[hub] = delay_begin[hub] + delay_ms;
  delay_next_state[hub] = next_state;
  state[hub] = STATE_DELAY_MS;
}

static bool is_transaction_locked() {
  return transaction_lock >= 0;
}

static bool lock_transaction(uint8_t hub) {
  if (is_transaction_locked())
    return false;
  transaction_lock = hub;
  if ((hub == 0 && (UHUB0_CTRL & bUH_LOW_SPEED) == 0) ||
      (hub == 1 && (UHUB1_CTRL & bUH_LOW_SPEED) == 0)) {
    USB_CTRL &= ~bUC_LOW_SPEED;
  } else {
    USB_CTRL |= bUC_LOW_SPEED;
  }
  return true;
}

static void unlock_transaction() {
  transaction_lock = -1;
}

static void host_transact_cont(uint8_t hub) {
  if ((transaction_ep_pid >> 4) != USB_PID_IN) {
    uint16_t send_size = (transaction_size < 64) ? transaction_size : 64;
    for (uint16_t i = 0; i < send_size; ++i)
      tx_buffer[i] = transaction_buffer[i];
    transaction_buffer += send_size;
    transaction_size -= send_size;
    UH_TX_LEN = send_size;
  } else {
    UH_TX_LEN = 0;
  }
  UH_EP_PID = transaction_ep_pid;
  UIF_TRANSFER = 0;
  state[hub] = STATE_TRANSACTION;
}

static void host_transact(
    uint8_t hub, uint8_t* buffer, uint16_t size, uint8_t recv_state, uint8_t ep, uint8_t pid, uint8_t tog) {
  transaction_buffer = buffer;
  transaction_size = size;
  transaction_recv_state = recv_state;
  transaction_ep_pid = (pid << 4) | (ep & 0x0f);
  UH_RX_CTRL = UH_TX_CTRL = tog;
  host_transact_cont(hub);
}

static void host_setup_transfer(
    uint8_t hub, uint8_t* buffer, uint16_t size, uint8_t recv_state) {
  host_transact(hub, buffer, size, recv_state, 0, USB_PID_SETUP, 0);
}

static void host_ack_transfer(uint8_t hub, uint8_t ep, uint8_t recv_state) {
  host_transact(hub, 0, 0, recv_state, ep, USB_PID_ACK, 0);
}

static void host_in_transfer(
    uint8_t hub, uint8_t ep, uint16_t size, uint8_t recv_state) {
  uint8_t tog = bUH_R_TOG | bUH_R_AUTO_TOG | bUH_T_TOG | bUH_T_AUTO_TOG;
  host_transact(hub, in_buffer, size, recv_state, ep, USB_PID_IN, tog);
}

static void host_out_transfer(
    uint8_t hub, uint8_t ep, uint16_t size, uint8_t recv_state) {
  uint8_t tog = bUH_R_TOG | bUH_R_AUTO_TOG | bUH_T_TOG | bUH_T_AUTO_TOG;
  host_transact(hub, in_buffer, size, recv_state, ep, USB_PID_OUT, tog);
}

static bool state_idle(uint8_t hub) {
  if ((hub == 0 && (USB_HUB_ST & bUHS_H0_ATTACH)) ||
      (hub == 1 && (USB_HUB_ST & bUHS_H1_ATTACH))) {
    // Wait 100ms for devices to be stable.
    delay_ms(hub, 100, STATE_CONNECT);
  }
  return false;
}

static bool state_connect(uint8_t hub) {
  // Reset the bus, and wait 15ms.
  // The device may be disconnected during the reset.
  resetting[hub] = true;
  if (!hub)
    UHUB0_CTRL = bUHS_BUS_RESET;
  else
    UHUB1_CTRL = bUHS_BUS_RESET;
  delay_ms(hub, 15, STATE_RESET);
  return false;
}

static bool state_reset(uint8_t hub) {
  // Stop resetting, and wait >250us.
  if (!hub)
    UHUB0_CTRL &= ~bUHS_BUS_RESET;
  else
    UHUB1_CTRL &= ~bUHS_BUS_RESET;
  delay_us(hub, 250, STATE_ENABLE);
  return false;
}

static bool state_enable(uint8_t hub) {
  // Wait til the device gets online again.
  if ((hub == 0 && (USB_HUB_ST & bUHS_H0_ATTACH) == 0) ||
      (hub == 1 && (USB_HUB_ST & bUHS_H1_ATTACH) == 0)) {
    return false;
  }
  resetting[hub] = false;

  if (!hub) {
    if (USB_HUB_ST & bUHS_DM_LEVEL)
      UHUB0_CTRL |= bUH_LOW_SPEED;
    UHUB0_CTRL |= bUH_PORT_EN;
  } else {
    if (USB_HUB_ST & bUHS_HM_LEVEL)
      UHUB1_CTRL |= bUH_LOW_SPEED;
    UHUB1_CTRL |= bUH_PORT_EN;
  }
  // Wait >200us.
  delay_us(hub, 200, STATE_GET_DEVICE_DESC);
  return false;
}

static bool state_get_device_desc(uint8_t hub) {
  if (!lock_transaction(hub))
    return false;
  if ((hub == 0 && (UHUB0_CTRL & bUH_LOW_SPEED) == 0) ||
      (hub == 1 && (UHUB1_CTRL & bUH_LOW_SPEED) == 0)) {
    USB_CTRL &= ~bUC_LOW_SPEED;
  } else {
    USB_CTRL |= bUC_LOW_SPEED;
  }
  host_setup_transfer(
      hub,
      (uint8_t*)&get_device_descriptor,
      sizeof(get_device_descriptor),
      STATE_GET_DEVICE_DESC_RECV);
  return false;
}

static bool state_get_device_desc_recv(uint8_t hub) {
  const struct usb_desc_device* desc = (const struct usb_desc_device*)in_buffer;
  if (usb_host->check_device_desc)
    usb_host->check_device_desc(hub, in_buffer);

  get_configuration_descriptor.wLength = 0x09;  // request the core part.
  is_hid[hub] = desc->bDeviceClass == USB_CLASS_HID;
  delay_us(hub, 250, STATE_GET_CONFIGURATION_DESC);
  return false;
}

static bool state_get_configuration_desc(uint8_t hub) {
  host_setup_transfer(
      hub,
      (uint8_t*)&get_configuration_descriptor,
      sizeof(get_configuration_descriptor),
      STATE_GET_CONFIGURATION_DESC_RECV);
  return false;
}

static bool state_get_configuration_desc_recv(uint8_t hub) {
  const struct usb_desc_configuration* desc = (const struct usb_desc_configuration*)in_buffer;
  if (get_configuration_descriptor.wLength != desc->wTotalLength) {
    get_configuration_descriptor.wLength = desc->wTotalLength;  // request full part.
    delay_us(hub, 250, STATE_GET_CONFIGURATION_DESC);
    return false;
  }
  if (usb_host->check_configuration_desc)
    usb_host->check_configuration_desc(hub, in_buffer);
  set_configuration_descriptor.wValue = desc->bConfigurationValue;
  // Note: multiple configurations are not supported.

  for (uint16_t offset = 0; offset < desc->wTotalLength; ) {
    const struct usb_desc_head* head =
        (const struct usb_desc_head*)(in_buffer + offset);
    if (head->bDescriptorType == USB_DESC_INTERFACE) {
      const struct usb_desc_interface* intf =
        (const struct usb_desc_interface*)(in_buffer + offset);
      if (intf->bInterfaceClass == USB_CLASS_HID)
        is_hid[hub] = true;
    } else if (head->bDescriptorType == USB_DESC_HID) {
      const struct usb_desc_hid* hid =
          (const struct usb_desc_hid*)(in_buffer + offset);
      get_hid_report_descriptor.wLength = hid->wDescriptorLength;
    }
    offset += head->bLength;
  }
  delay_us(hub, 250, STATE_SET_CONFIGURATION);
  return false;
}

static bool state_set_configuration(uint8_t hub) {
  host_setup_transfer(
      hub,
      (uint8_t*)&set_configuration_descriptor,
      sizeof(set_configuration_descriptor),
      is_hid[hub] ? STATE_HID_SETUP : STATE_DONE);
  return false;
}

static bool state_hid_setup(uint8_t hub) {
  delay_us(hub, 250, STATE_GET_HID_REPORT_DESC);
  return false;
}

static bool state_get_hid_report_desc(uint8_t hub) {
  host_setup_transfer(
      hub,
      (uint8_t*)&get_hid_report_descriptor,
      sizeof(get_hid_report_descriptor),
      STATE_GET_HID_REPORT_DESC_RECV);
  return false;
}

static bool state_get_hid_report_desc_recv(uint8_t hub) {
  if (usb_host->check_hid_report_desc)
    usb_host->check_hid_report_desc(hub, in_buffer);
  unlock_transaction();
  delay_us(hub, 250, STATE_READY);
  return false;
}

static bool state_done(uint8_t hub) {
  delay_us(hub, 250, STATE_READY);
  return false;
}

static bool state_ready(uint8_t hub) {
  hub;
  return false;
}

static bool state_in_recv(uint8_t hub) {
  if (usb_host->in)
    usb_host->in(hub, in_buffer);
  unlock_transaction();
  delay_us(hub, 250, STATE_READY);
  return false;
}

static bool state_delay_us(uint8_t hub) {
  if (timer3_tick_raw_between(delay_begin[hub], delay_end[hub]))
    return false;
  state[hub] = delay_next_state[hub];
  return true;
}

static bool state_delay_ms(uint8_t hub) {
  if (timer3_tick_msec_between(delay_begin[hub], delay_end[hub]))
    return false;
  state[hub] = delay_next_state[hub];
  return true;
}

static bool state_transaction(uint8_t hub) {
  // Wait until the transaction completes.
  if (!UIF_TRANSFER)
    return false;

  UH_EP_PID = 0;  // Stop USB transaction.

  if ((transaction_ep_pid >> 4) == USB_PID_IN) {
    uint16_t size = (transaction_size > USB_RX_LEN)
        ? USB_RX_LEN : transaction_size;
    for (uint16_t i = 0; i < size; ++i)
      transaction_buffer[i] = rx_buffer[i];
    transaction_buffer = &transaction_buffer[size];
    transaction_size -= size;
  }

  uint8_t token = USB_INT_ST & MASK_UIS_HRES;
  if (U_TOG_OK || token == USB_PID_DATA0 || token == USB_PID_DATA1) {
    if (transaction_size) {
      delay_us(hub, 250, STATE_TRANSACTION_CONT);
      return false;
    }

    // Succeeded.
    uint8_t pid = transaction_ep_pid >> 4;
    if (pid == USB_PID_SETUP) {
      const struct usb_setup_req* req = (const struct usb_setup_req*)tx_buffer;
      if (req->wLength &&
          (req->bRequestType & USB_REQ_DIR_MASK) == USB_REQ_DIR_IN) {
        delay_us(hub, 250, STATE_TRANSACTION_IN);
        return false;
      } else if (req->wLength &&
          (req->bRequestType & USB_REQ_DIR_MASK) == USB_REQ_DIR_OUT) {
        halt("out");
      }
    } else if (pid == USB_PID_IN) {
      delay_us(hub, 250, STATE_TRANSACTION_ACK);
      return false;
    }
    state[hub] = transaction_recv_state;
    return true;
  }
  if (token == USB_PID_NAK) {
    delay_us(hub, 250, STATE_TRANSACTION_RETRY);
    return false;
  }

  Serial.printf("\ntransmit error: %x\n", USB_INT_ST);
  halt("");
  return false;
}

static bool state_transaction_in(uint8_t hub) {
  const struct usb_setup_req* req = (const struct usb_setup_req*)tx_buffer;
  const uint8_t ep = transaction_ep_pid & 0x0f;
  host_in_transfer(hub, ep, req->wLength, transaction_recv_state);
  return false;
}

static bool state_transaction_ack(uint8_t hub) {
  const uint8_t ep = transaction_ep_pid & 0x0f;
  host_ack_transfer(hub, ep, transaction_recv_state);
  return false;
}

static bool state_transaction_cont(uint8_t hub) {
  host_transact_cont(hub);
  return false;
}

static bool state_transaction_retry(uint8_t hub) {
  UH_EP_PID = transaction_ep_pid;
  UIF_TRANSFER = 0;
  state[hub] = STATE_TRANSACTION;
  return false;
}

static bool fsm(uint8_t hub) {
  if (state[hub] != STATE_IDLE && !resetting[hub]) {
    if (hub == 0 && (USB_HUB_ST & bUHS_H0_ATTACH) == 0) {
      UHUB0_CTRL = 0x00;
      state[hub] = STATE_IDLE;
    }
    if (hub == 1 && (USB_HUB_ST & bUHS_H1_ATTACH) == 0) {
      UHUB1_CTRL = 0x00;
      state[hub] = STATE_IDLE;
    }
    if (state[hub] == STATE_IDLE) {
      UH_EP_PID = 0;  // Stop USB transaction.
      USB_CTRL = bUC_HOST_MODE | bUC_INT_BUSY | bUC_DMA_EN;
      if (usb_host->disconnected)
        usb_host->disconnected(hub);
    }
  }
  switch (state[hub]) {
    case STATE_IDLE:
      return state_idle(hub);
    case STATE_CONNECT:
      return state_connect(hub);
    case STATE_RESET:
      return state_reset(hub);
    case STATE_ENABLE:
      return state_enable(hub);
    case STATE_GET_DEVICE_DESC:
      return state_get_device_desc(hub);
    case STATE_GET_DEVICE_DESC_RECV:
      return state_get_device_desc_recv(hub);
    case STATE_GET_CONFIGURATION_DESC:
      return state_get_configuration_desc(hub);
    case STATE_GET_CONFIGURATION_DESC_RECV:
      return state_get_configuration_desc_recv(hub);
    case STATE_SET_CONFIGURATION:
      return state_set_configuration(hub);
    case STATE_HID_SETUP:
      return state_hid_setup(hub);
    case STATE_GET_HID_REPORT_DESC:
      return state_get_hid_report_desc(hub);
    case STATE_GET_HID_REPORT_DESC_RECV:
      return state_get_hid_report_desc_recv(hub);
    case STATE_READY:
      return state_ready(hub);
    case STATE_IN_RECV:
      return state_in_recv(hub);
    case STATE_DELAY_US:
      return state_delay_us(hub);
    case STATE_DELAY_MS:
      return state_delay_ms(hub);
    case STATE_TRANSACTION:
      return state_transaction(hub);
    case STATE_TRANSACTION_IN:
      return state_transaction_in(hub);
    case STATE_TRANSACTION_ACK:
      return state_transaction_ack(hub);
    case STATE_TRANSACTION_CONT:
      return state_transaction_cont(hub);
    case STATE_TRANSACTION_RETRY:
      return state_transaction_retry(hub);
    default:
      halt("unknown state");
  }
  return false;
}

void usb_host_init(struct usb_host* host) {
  usb_host = host;

  // DMA addresses must be even
  if ((uint16_t)rx_buffer & 1)
    rx_buffer++;
  if ((uint16_t)tx_buffer & 1)
    tx_buffer++;
  IE_USB = 0;  // Disable USB interrupts
  USB_CTRL = bUC_HOST_MODE;
  USB_DEV_AD = 0x00;
  UH_EP_MOD = bUH_EP_TX_EN | bUH_EP_RX_EN;  // Enable host EP TX and RX
  UH_RX_DMA_H = (uint16_t)rx_buffer >> 8;
  UH_RX_DMA_L = (uint16_t)rx_buffer & 0xff;
  UH_TX_DMA_H = (uint16_t)tx_buffer >> 8;
  UH_TX_DMA_L = (uint16_t)tx_buffer & 0xff;
  UH_RX_CTRL = 0x00;
  UH_TX_CTRL = 0x00;
  USB_CTRL = bUC_HOST_MODE | bUC_INT_BUSY | bUC_DMA_EN;
  UH_SETUP = bUH_SOF_EN;  // Enable automatic SOF
  USB_INT_FG = 0xff;  // Clear interrupt flags
  if (host->flags & USE_HUB0)
    UHUB0_CTRL = 0x00;  // Enable HUB0
  if (host->flags & USE_HUB1)
    UHUB1_CTRL = 0x00;  // Enable HUB1
  USB_INT_EN = bUIE_TRANSFER | bUIE_DETECT;

  for (uint8_t i = 0; i < 2; ++i)
    state[i] = STATE_IDLE;
  
  timer3_tick_init();
}

void usb_host_poll() {
  if (usb_host->flags & USE_HUB0)
    while (fsm(0));
  if (usb_host->flags & USE_HUB1)
    while (fsm(1));
}

bool usb_host_ready(uint8_t hub) {
  return state[hub] == STATE_READY && !is_transaction_locked();
}

bool usb_host_in(uint8_t hub, uint8_t ep, uint8_t size) {
  if (!usb_host_ready(hub) || !lock_transaction(hub))
    return false;
  host_in_transfer(hub, ep, size, STATE_IN_RECV);
  return false;
}