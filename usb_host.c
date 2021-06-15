// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "usb_host.h"

#include "ch559.h"
#include "io.h"
#include "usb.h"

enum {
  STATE_IDLE,

  STATE_CONNECT,
  STATE_RESET,
  STATE_ENABLE,
  STATE_GET_DEVICE_DESC,
  STATE_GET_DEVICE_DESC_IN,
  STATE_GET_DEVICE_DESC_PARSE,

  STATE_DISCONNECT,

  STATE_DELAY,
  STATE_TRANSACTION,
};

static const struct usb_setup_req get_device_descriptor = {
  USB_REQ_TYP_IN,
  USB_GET_DESCRIPTOR,
  USB_DESC_DEVICE << 8,
  0x00,
  0x12,
};
static const struct usb_setup_req get_configuration_descriptor = {
  USB_REQ_TYP_IN,
  USB_GET_DESCRIPTOR,
  USB_DESC_CONFIGURATION << 8,
  0x00,
  0x09,
};
static struct usb_host* usb_host = 0;

static uint8_t _rx_buffer[64 + 1];
static uint8_t* rx_buffer = _rx_buffer;
static uint8_t in_buffer[256];
static uint8_t _tx_buffer[64 + 1];
static uint8_t* tx_buffer = _tx_buffer;

static uint8_t* transaction_buffer[2] = { 0, 0 };
static uint8_t transaction_size[2] = { 0, 0 };
static uint8_t transaction_recv_state[2] = { 0, 0 };
static uint8_t transaction_ep_pid[2] = { 0, 0 };
static uint8_t transaction_tog[2] = { 0, 0 };

static uint8_t state[2] = { STATE_IDLE, STATE_IDLE };
static uint16_t delay_until[2] = { 0, 0 };
static uint8_t delay_next_state[2] = { 0, 0 };
static bool resetting[2] = { false, false };

static void halt(const char* message) {
  Serial.println(message);
  for (;;);
}

static void delay_ms(uint8_t hub, uint16_t delay_ms, uint8_t next_state) {
  if (delay_until[0] == 0 && delay_until[1] == 0)
    timer3_tick_reset();
  delay_until[hub] = timer3_tick_msec() + delay_ms;
  delay_next_state[hub] = next_state;
  state[hub] = STATE_DELAY;
}

static void host_transact(
    uint8_t hub, uint8_t* buffer, uint8_t size, uint8_t recv_state, uint8_t ep, uint8_t pid, uint8_t tog) {
  transaction_buffer[hub] = buffer;
  transaction_size[hub] = size;
  transaction_recv_state[hub] = recv_state;
  transaction_ep_pid[hub] = (pid << 4) | ep;
  transaction_tog[hub] = tog;
  uint8_t alt_hub = (hub + 1) & 1;
  if (transaction_size[alt_hub]) {
    halt("hub conflict");
  }
  if (pid != USB_PID_IN) {
    uint8_t send_size = (size < 64) ? size : 64;
    transaction_buffer[hub] = &buffer[send_size];
    transaction_size[hub] = size - send_size;
    for (uint8_t i = 0; i < send_size; ++i)
      tx_buffer[i] = buffer[i];

    Serial.println("=== transact dump ===");
    for (uint8_t i = 0; i < send_size; ++i) {
      Serial.printc(tx_buffer[i], HEX);
      if ((i & 0xf) == 0xf)
        Serial.println("");
      else
        Serial.print(", ");
    }
    Serial.println("\r\n===");

    UH_TX_LEN = send_size;
  } else {
    UH_TX_LEN = 0;
  }
  UH_RX_CTRL = UH_TX_CTRL = tog;
  UH_EP_PID = transaction_ep_pid[hub];
  UIF_TRANSFER = 0;
  state[hub] = STATE_TRANSACTION;
}

static void host_setup_transfer(
    uint8_t hub, uint8_t* buffer, uint8_t size, uint8_t recv_state) {
  host_transact(hub, buffer, size, recv_state, 0, USB_PID_SETUP, 0);
}

static void host_in_transfer(uint8_t hub, uint8_t size, uint8_t recv_state) {
  uint8_t tog = bUH_R_TOG | bUH_R_AUTO_TOG | bUH_T_TOG | bUH_T_AUTO_TOG;
  host_transact(hub, in_buffer, size, recv_state, 0, USB_PID_IN, tog);
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
  delay_ms(hub, 1, STATE_ENABLE);
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
  delay_ms(hub, 1, STATE_GET_DEVICE_DESC);
  return false;
}

static bool state_get_device_desc(uint8_t hub) {
  host_setup_transfer(
      hub,
      (uint8_t*)&get_device_descriptor,
      sizeof(get_device_descriptor),
      STATE_GET_DEVICE_DESC_IN);
  return false;
}

static bool state_get_device_desc_in(uint8_t hub) {
  host_in_transfer(
      hub, get_device_descriptor.wLength, STATE_GET_DEVICE_DESC_PARSE);
  return false;
}

static bool state_get_device_desc_parse(uint8_t hub) {
  const struct usb_desc_device* desc = (const struct usb_desc_device*)in_buffer;
  usb_host->check_device_desc(hub, in_buffer);
  halt("");
  return false;
}

static bool state_disconnect(uint8_t hub) {
  if (!hub)
    UHUB0_CTRL = 0x00;
  else
    UHUB1_CTRL = 0x00;
  state[hub] = STATE_IDLE;
  return true;
}

static bool state_delay(uint8_t hub) {
  if (!timer3_tick_gt(delay_until[hub]))
    return false;
  delay_until[hub] = 0;
  state[hub] = delay_next_state[hub];
  return true;
}

static bool state_transact(uint8_t hub) {
  // Wait until the transaction completes.
  if (!UIF_TRANSFER)
    return false;
  UH_EP_PID = 0;  // Stop USB transaction.

  if ((transaction_ep_pid[hub] >> 4) == USB_PID_IN) {
    uint8_t size = (transaction_size[hub] > USB_RX_LEN) ? USB_RX_LEN
                                                        : transaction_size[hub];
    for (uint8_t i = 0; i < size; ++i)
      transaction_buffer[hub][i] = rx_buffer[i];
    transaction_buffer[hub] = &transaction_buffer[hub][size];
    transaction_size[hub] -= size;
  }

  if (transaction_size[hub]) {
    halt("transaction cont");
    return false;
  }
  if (U_TOG_OK) {
    // Succeeded.
    state[hub] = transaction_recv_state[hub];
    return true;
  }
  Serial.printc(USB_INT_ST, HEX);
  halt("\ntransmit error");
  return false;
}

static bool fsm(uint8_t hub) {
  if (state[hub] != STATE_IDLE && !resetting[hub]) {
    if ((hub == 0 && (USB_HUB_ST & bUHS_H0_ATTACH) == 0) ||
        (hub == 1 && (USB_HUB_ST & bUHS_H1_ATTACH) == 0)) {
      state[hub] = STATE_DISCONNECT;
      return true;
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
    case STATE_GET_DEVICE_DESC_IN:
      return state_get_device_desc_in(hub);
    case STATE_GET_DEVICE_DESC_PARSE:
      return state_get_device_desc_parse(hub);
    case STATE_DISCONNECT:
      return state_disconnect(hub);
    case STATE_DELAY:
      return state_delay(hub);
    case STATE_TRANSACTION:
      return state_transact(hub);
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