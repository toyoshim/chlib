// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "usb_host.h"

#include "ch559.h"
#include "io.h"
#include "serial.h"

// #define _USB_HOST_DBG_LOG
// #define _IMPL_USB_HOST_LOG_SEND
// #define _IMPL_USB_HOST_LOG_RECV
// #define _IMPL_USB_HOST_LOG_STALL
// #define _IMPL_USB_HOST_LOG_NAK

enum {
  STATE_IDLE,

  STATE_CONNECT,
  STATE_RESET,
  STATE_ENABLE,
  STATE_SET_ADDRESS,
  STATE_SET_ADDRESS_DONE,
  STATE_GET_DEVICE_DESC,
  STATE_GET_DEVICE_DESC_RECV,
  STATE_GET_STRING_DESC,
  STATE_GET_STRING_DESC_RECV,
  STATE_GET_CONFIGURATION_DESC,
  STATE_GET_CONFIGURATION_DESC_RECV,
  STATE_GET_FULL_CONFIGURATION_DESC,
  STATE_GET_FULL_CONFIGURATION_DESC_RECV,
  STATE_SET_CONFIGURATION,
  STATE_SET_CONFIGURATION_DONE,
  STATE_SET_FEATURE,
  STATE_EXTRA_SETUP,
  STATE_GET_HID_REPORT_DESC,
  STATE_GET_HID_REPORT_DESC_RECV,
  STATE_HID_SET_PROTOCOL,
  STATE_GET_HUB_DESC,
  STATE_GET_HUB_DESC_RECV,
  STATE_SET_PORT_POWER_FEATURE,
  STATE_SET_PORT_POWER_FEATURE_DONE,
  STATE_CLEAR_PORT_CONNECTION_FEATURE,
  STATE_CLEAR_PORT_CONNECTION_FEATURE_DONE,

  STATE_DONE,
  STATE_READY,
  STATE_HALT,

  STATE_IN_RECV,
  STATE_OUT_DONE,
  STATE_HID_GET_REPORT,

  STATE_DELAY_US,
  STATE_DELAY_MS,
  STATE_TRANSACTION,
  STATE_TRANSACTION_IN,
  STATE_TRANSACTION_OUT,
  STATE_TRANSACTION_ACK,
  STATE_TRANSACTION_CONT,
  STATE_TRANSACTION_RETRY,
};

#define AUTO_TOGGLE (bUH_R_TOG | bUH_R_AUTO_TOG | bUH_T_TOG | bUH_T_AUTO_TOG)

static struct usb_setup_req set_address_descriptor = {
    USB_REQ_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    USB_SET_ADDRESS,
    0x0000,  // address: can be modified
    0x0000,
    0x0000,
};
static struct usb_setup_req get_device_descriptor = {
    USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    USB_GET_DESCRIPTOR,
    USB_DESC_DEVICE << 8,
    0x0000,
    0x0012,  // requesting descriptor size: can be modified
};
static struct usb_setup_req get_string_descriptor = {
    USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    USB_GET_DESCRIPTOR,
    USB_DESC_STRING << 8,
    0x0409,
    0x0002,  // requesting descriptor size: can be modified
};
static struct usb_setup_req get_configuration_descriptor = {
    USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    USB_GET_DESCRIPTOR,
    USB_DESC_CONFIGURATION << 8,
    0x0000,
    0x0009,  // requesting descriptor size: can be modified
};
static struct usb_setup_req set_configuration_descriptor = {
    USB_REQ_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    USB_SET_CONFIGURATION,
    0x0000,  // configuration id: can be modified
    0x0000,
    0x0000,
};
static const struct usb_setup_req set_feature_descriptor = {
    USB_REQ_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_DEVICE,
    USB_SET_FEATURE,
    0x0001,  // DEVICE_REMOTE_WAKEUP
    0x0000,
    0x0000,
};
static struct usb_setup_req get_hid_report_descriptor = {
    USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECPT_INTERFACE,
    USB_GET_DESCRIPTOR,
    USB_DESC_HID_REPORT << 8,
    0x0000,
    0x0000,  // requesting descriptor size: can be modified
};
static struct usb_setup_req hid_get_report = {
    USB_REQ_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_INTERFACE,
    USB_HID_GET_REPORT,
    (1 << 8) | 0,  // report type, report id: can be modified
    0x0000,        // interface: can be modified
    0x0000,        // report length: can be modified
};
static struct usb_setup_req hid_set_protocol = {
    USB_REQ_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_INTERFACE,
    USB_HID_SET_PROTOCOL,
    0,       // Boot protocol
    0x0000,  // interface
    0x0000,
};
static struct usb_setup_req get_hub_descriptor = {
    USB_REQ_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_DEVICE,
    USB_GET_DESCRIPTOR,
    USB_DESC_HUB << 8,
    0x0000,
    0x0000,  // requesting descriptor size: can be modified
};
static struct usb_setup_req set_port_power_feature = {
    USB_REQ_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_OTHER,
    USB_SET_FEATURE,
    USB_FEATURE_PORT_POWER,
    0x0000,  // Port: can be modified
    0x0000,
};
static struct usb_setup_req clear_port_connection_feature = {
    USB_REQ_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_OTHER,
    USB_CLEAR_FEATURE,
    USB_FEATURE_C_PORT_CONNECTION,
    0x0000,  // Port: can be modified
    0x0000,
};

static struct usb_host* usb_host = 0;

static uint8_t _rx_buffer[64 + 1];
static uint8_t* rx_buffer = _rx_buffer;
static uint8_t buffer[1024];
static uint8_t _tx_buffer[64 + 1];
static uint8_t* tx_buffer = _tx_buffer;
static uint16_t ep_max_packet_size[2][16];

static int8_t transaction_lock = -1;
static uint8_t* transaction_buffer = 0;
static uint16_t transaction_size = 0;
static uint8_t transaction_recv_state = STATE_IDLE;
static uint8_t transaction_ep_pid = 0;
static uint8_t transaction_stage = 0;  // 0: setup, 1: status, 2: non-setup data

static uint8_t state[2] = {STATE_IDLE, STATE_IDLE};
static bool initial_check[2] = {false, false};
static uint16_t delay_begin[2] = {0, 0};
static uint16_t delay_end[2] = {0, 0};
static uint8_t delay_next_state[2] = {0, 0};
static bool resetting[2] = {false, false};
static bool no_remote_wakeup[2] = {false, false};
static bool is_hid[2] = {false, false};
static bool is_hid_boot[2] = {false, false};
static uint8_t hub_ports[2] = {0, 0};
static uint8_t hub_address[2] = {0, 0};
static uint8_t hid_interface_number[2] = {0, 0};
static bool do_not_retry[2] = {false, false};
static uint16_t user_request_size = 0;
static uint8_t string_index[3] = {0, 0, 0};

void usb_host_log_send(uint8_t ep, uint8_t pid, uint8_t size, uint8_t* buffer);
void usb_host_log_recv(uint8_t ep, uint8_t pid, uint8_t size, uint8_t* buffer);
void usb_host_log_stall(void);
void usb_host_log_nak(void);

static void halt(uint8_t hub) {
  state[hub] = STATE_HALT;
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

static bool is_transaction_locked(void) {
  return transaction_lock >= 0;
}

static bool lock_transaction(uint8_t hub, uint8_t target_device_addr) {
  if (is_transaction_locked())
    return false;
  transaction_lock = hub;
  if ((hub == 0 && (UHUB0_CTRL & bUH_LOW_SPEED) == 0) ||
      (hub == 1 && (UHUB1_CTRL & bUH_LOW_SPEED) == 0)) {
    USB_CTRL &= ~bUC_LOW_SPEED;
  } else {
    USB_CTRL |= bUC_LOW_SPEED;
  }
  USB_DEV_AD = target_device_addr;
  return true;
}

static bool unlock_transaction(uint8_t hub) {
  if (transaction_lock != (int8_t)hub)
    return false;
  transaction_lock = -1;
  return true;
}

static bool find_string_index(uint8_t* index) {
  for (uint8_t i = 0; i < 3; ++i) {
    if (string_index[i]) {
      *index = i;
      return true;
    }
  }
  return false;
}

static void host_transact_cont(uint8_t hub, uint8_t tog) {
  uint16_t size;
  if ((transaction_ep_pid >> 4) != USB_PID_IN) {
    size = (transaction_size < 64) ? transaction_size : 64;
    for (uint16_t i = 0; i < size; ++i)
      tx_buffer[i] = transaction_buffer[i];
    transaction_buffer += size;
    transaction_size -= size;
    UH_TX_LEN = size;
  } else {
    size = transaction_size;
    UH_TX_LEN = 0;
  }

#ifdef _USB_HOST_DBG_LOG
  usb_host_log_send(transaction_ep_pid & 0x0f, transaction_ep_pid >> 4, size,
                    tx_buffer);
#endif  // _USB_HOST_DBG_LOG

  UH_EP_PID = transaction_ep_pid;
  UH_RX_CTRL = UH_TX_CTRL = tog;
  UIF_TRANSFER = 0;
  state[hub] = STATE_TRANSACTION;
}

static void host_transact(uint8_t hub,
                          uint8_t* buffer,
                          uint16_t size,
                          uint8_t recv_state,
                          uint8_t ep,
                          uint8_t pid,
                          uint8_t tog) {
  transaction_buffer = buffer;
  transaction_size = size;
  transaction_recv_state = recv_state;
  transaction_ep_pid = (pid << 4) | (ep & 0x0f);
  host_transact_cont(hub, tog);
}

static void host_setup_transfer(uint8_t hub,
                                uint8_t* buffer,
                                uint16_t size,
                                uint8_t recv_state) {
  transaction_stage = 0;
  host_transact(hub, buffer, size, recv_state, 0, USB_PID_SETUP, 0);
}

static void host_in_transfer(uint8_t hub,
                             uint8_t ep,
                             uint16_t size,
                             uint8_t recv_state,
                             uint8_t tog) {
  host_transact(hub, buffer, size, recv_state, ep, USB_PID_IN, tog);
}

static void host_out_transfer(uint8_t hub,
                              uint8_t ep,
                              uint8_t* buffer,
                              uint16_t size,
                              uint8_t recv_state,
                              uint8_t tog) {
  host_transact(hub, buffer, size, recv_state, ep, USB_PID_OUT, tog);
}

static void host_ack_transfer(uint8_t hub,
                              uint8_t ep,
                              uint8_t* buffer,
                              uint16_t size,
                              uint8_t recv_state) {
  host_transact(hub, buffer, size, recv_state, ep, USB_PID_ACK, 0);
}

static bool state_idle(uint8_t hub) {
  if ((hub == 0 && (USB_HUB_ST & bUHS_H0_ATTACH)) ||
      (hub == 1 && (USB_HUB_ST & bUHS_H1_ATTACH))) {
    // Wait 500ms (>100ms) for devices to be stable.
    delay_ms(hub, 500, STATE_CONNECT);
  }
  return false;
}

static bool state_connect(uint8_t hub) {
  // Reset the bus, and wait 100ms (>15ms).
  // The device may be disconnected during the reset.
  resetting[hub] = true;
  if (!hub)
    UHUB0_CTRL = bUHS_BUS_RESET;
  else
    UHUB1_CTRL = bUHS_BUS_RESET;
  initial_check[hub] = true;
  hub_address[hub] = 0;
  delay_ms(hub, 100, STATE_RESET);
  return false;
}

static bool state_reset(uint8_t hub) {
  // Stop resetting, and wait >250us.
  if (!hub)
    UHUB0_CTRL &= ~bUHS_BUS_RESET;
  else
    UHUB1_CTRL &= ~bUHS_BUS_RESET;
  delay_us(hub, 500, STATE_ENABLE);
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
  delay_us(hub, 250,
           initial_check[hub] ? STATE_GET_DEVICE_DESC : STATE_SET_ADDRESS);
  return false;
}

static bool state_set_address(uint8_t hub) {
  if (!lock_transaction(hub, 0))
    return false;

  set_address_descriptor.wValue =
      hub_address[hub] ? hub_address[hub] : (1 + hub);
  host_setup_transfer(hub, (uint8_t*)&set_address_descriptor,
                      sizeof(set_address_descriptor), STATE_SET_ADDRESS_DONE);
  return false;
}

static bool state_set_address_done(uint8_t hub) {
  unlock_transaction(hub);
  lock_transaction(hub, set_address_descriptor.wValue);
  // Wait >2 ms
  delay_ms(hub, 2, STATE_GET_DEVICE_DESC);
  return false;
}

static bool state_get_device_desc(uint8_t hub) {
  if (initial_check[hub] && !lock_transaction(hub, 0))
    return false;

  get_device_descriptor.wLength = initial_check[hub] ? 0x08 : 0x12;
  host_setup_transfer(hub, (uint8_t*)&get_device_descriptor,
                      sizeof(get_device_descriptor),
                      STATE_GET_DEVICE_DESC_RECV);
  return false;
}

static bool state_get_device_desc_recv(uint8_t hub) {
  const struct usb_desc_device* desc = (const struct usb_desc_device*)buffer;
  if (initial_check[hub]) {
    // Initial check is done. Let's try full boot.
    unlock_transaction(hub);
    state_connect(hub);
    initial_check[hub] = false;
    ep_max_packet_size[hub][0] = desc->bMaxPacketSize0;
    delay_ms(hub, 15, STATE_RESET);
    return false;
  }
  if (usb_host->check_device_desc)
    usb_host->check_device_desc(hub, buffer);

  ep_max_packet_size[hub][0] = desc->bMaxPacketSize0;
  is_hid[hub] = desc->bDeviceClass == USB_CLASS_HID;
  is_hid_boot[hub] = desc->bDeviceClass == USB_CLASS_HID &&
                     desc->bDeviceSubClass == USB_HID_SUBCLASS_BOOT;
  hub_ports[hub] = (desc->bDeviceClass == USB_CLASS_HUB) ? 1 : 0;
  string_index[0] = desc->iManufacturer;
  string_index[1] = desc->iProduct;
  string_index[2] = desc->iSerialNumber;

  // Setup requests to ask the core part for the first request.
  get_string_descriptor.wLength = 0x0002;
  get_configuration_descriptor.wLength = 0x0009;
  get_hub_descriptor.wLength = 0x0008;

  delay_us(hub, 250, STATE_GET_STRING_DESC);
  return false;
}

static bool state_get_string_desc(uint8_t hub) {
  uint8_t i = 0;
  if (!find_string_index(&i)) {
    state[hub] = STATE_GET_CONFIGURATION_DESC;
    return true;
  }
  get_string_descriptor.wValue = (USB_DESC_STRING << 8) | string_index[i];
  host_setup_transfer(hub, (uint8_t*)&get_string_descriptor,
                      sizeof(get_string_descriptor),
                      STATE_GET_STRING_DESC_RECV);
  return false;
}

static bool state_get_string_desc_recv(uint8_t hub) {
  if (get_string_descriptor.wLength == 2) {
    const struct usb_desc_head* head = (const struct usb_desc_head*)buffer;
    if (head->bLength != 2) {
      // Request full part.
      get_string_descriptor.wLength = head->bLength;
      delay_us(hub, 250, STATE_GET_STRING_DESC);
      return false;
    }
  }

  uint8_t i = 0;
  find_string_index(&i);

  if (usb_host->check_string_desc)
    usb_host->check_string_desc(hub, string_index[i], buffer);

  string_index[i] = 0;
  if (find_string_index(&i)) {
    // Request core part.
    get_string_descriptor.wLength = 2;
    delay_us(hub, 250, STATE_GET_STRING_DESC);
    return false;
  }
  delay_us(hub, 250, STATE_GET_CONFIGURATION_DESC);
  return false;
}

static bool state_get_configuration_desc(uint8_t hub) {
  host_setup_transfer(hub, (uint8_t*)&get_configuration_descriptor,
                      sizeof(get_configuration_descriptor),
                      STATE_GET_CONFIGURATION_DESC_RECV);
  return false;
}

static bool state_get_configuration_desc_recv(uint8_t hub) {
  const struct usb_desc_configuration* desc =
      (const struct usb_desc_configuration*)buffer;
  if (get_configuration_descriptor.wLength != desc->wTotalLength) {
    // Request full part.
    get_configuration_descriptor.wLength = desc->wTotalLength;
    delay_us(hub, 250, STATE_GET_CONFIGURATION_DESC);
    return false;
  }
  no_remote_wakeup[hub] = (desc->bmAttributes & 0x20) == 0;
  if (usb_host->check_configuration_desc)
    usb_host->check_configuration_desc(hub, buffer);
  set_configuration_descriptor.wValue = desc->bConfigurationValue;
  // Note: multiple configurations are not supported.

  for (uint8_t i = 1; i < 16; ++i)
    ep_max_packet_size[hub][i] = 0;

  for (uint16_t offset = 0; offset < desc->wTotalLength;) {
    const struct usb_desc_head* head =
        (const struct usb_desc_head*)(buffer + offset);
    if (head->bDescriptorType == USB_DESC_INTERFACE) {
      const struct usb_desc_interface* intf =
          (const struct usb_desc_interface*)(buffer + offset);
      if (intf->bInterfaceClass == USB_CLASS_HID) {
        is_hid[hub] = true;
        if (intf->bInterfaceSubClass == USB_HID_SUBCLASS_BOOT)
          is_hid_boot[hub] = true;
      }
      if (is_hid[hub])
        hid_interface_number[hub] = intf->bInterfaceNumber;
    } else if (head->bDescriptorType == USB_DESC_HID) {
      const struct usb_desc_hid* hid =
          (const struct usb_desc_hid*)(buffer + offset);
      get_hid_report_descriptor.wLength = hid->wDescriptorLength;
    } else if (head->bDescriptorType == USB_DESC_ENDPOINT) {
      const struct usb_desc_endpoint* ep =
          (const struct usb_desc_endpoint*)(buffer + offset);
      ep_max_packet_size[hub][ep->bEndpointAddress & 0x0f] = ep->wMaxPacketSize;
    }
    offset += head->bLength;
  }
  delay_ms(hub, 5, STATE_SET_CONFIGURATION);
  return false;
}

static bool state_set_configuration(uint8_t hub) {
  host_setup_transfer(hub, (uint8_t*)&set_configuration_descriptor,
                      sizeof(set_configuration_descriptor),
                      STATE_SET_CONFIGURATION_DONE);
  return false;
}

static bool state_set_configuration_done(uint8_t hub) {
  delay_us(hub, 250, STATE_SET_FEATURE);
  return false;
}

static bool state_set_feature(uint8_t hub) {
  uint8_t next_state =
      (is_hid[hub] || hub_ports[hub]) ? STATE_EXTRA_SETUP : STATE_DONE;
  if (no_remote_wakeup[hub]) {
    state[hub] = next_state;
    return true;
  }
  host_setup_transfer(hub, (uint8_t*)&set_feature_descriptor,
                      sizeof(set_feature_descriptor), next_state);
  return false;
}

static bool state_extra_setup(uint8_t hub) {
  delay_us(hub, 250,
           hub_ports[hub]     ? STATE_GET_HUB_DESC
           : is_hid_boot[hub] ? STATE_HID_SET_PROTOCOL
                              : STATE_GET_HID_REPORT_DESC);
  return false;
}

static bool state_get_hid_report_desc(uint8_t hub) {
  host_setup_transfer(hub, (uint8_t*)&get_hid_report_descriptor,
                      sizeof(get_hid_report_descriptor),
                      STATE_GET_HID_REPORT_DESC_RECV);
  return false;
}

static bool state_get_hid_report_desc_recv(uint8_t hub) {
  if (usb_host->check_hid_report_desc)
    usb_host->check_hid_report_desc(hub, buffer);
  unlock_transaction(hub);
  delay_ms(hub, 5, STATE_READY);
  return false;
}

static bool state_hid_set_protocol(uint8_t hub) {
  host_setup_transfer(hub, (uint8_t*)&hid_set_protocol,
                      sizeof(hid_set_protocol), STATE_DONE);
  return false;
}

static bool state_get_hub_desc(uint8_t hub) {
  host_setup_transfer(hub, (uint8_t*)&get_hub_descriptor,
                      sizeof(get_hub_descriptor), STATE_GET_HUB_DESC_RECV);
  return false;
}

static bool state_get_hub_desc_recv(uint8_t hub) {
  const struct usb_desc_hub* desc = (const struct usb_desc_hub*)buffer;
  if (get_hub_descriptor.wLength != desc->bDescLength) {
    // Request full part.
    get_hub_descriptor.wLength = desc->bDescLength;
    delay_us(hub, 250, STATE_GET_HUB_DESC);
    return false;
  }
  hub_ports[hub] = desc->bNbPorts;
  set_port_power_feature.wIndex = 1;  // port index originated from 1.
  delay_ms(hub, 5, STATE_SET_PORT_POWER_FEATURE);
  return false;
}

static bool state_set_port_power_feature(uint8_t hub) {
  if (set_port_power_feature.wIndex > hub_ports[hub]) {
    clear_port_connection_feature.wIndex = 1;  // port index originated from 1.
    delay_ms(hub, 5, STATE_CLEAR_PORT_CONNECTION_FEATURE);
    return false;
  }
  host_setup_transfer(hub, (uint8_t*)&set_port_power_feature,
                      sizeof(set_port_power_feature),
                      STATE_SET_PORT_POWER_FEATURE_DONE);
  return false;
}

static bool state_set_port_power_feature_done(uint8_t hub) {
  set_port_power_feature.wIndex++;
  delay_ms(hub, 5, STATE_SET_PORT_POWER_FEATURE);
  return false;
}

static bool state_clear_port_connection_feature(uint8_t hub) {
  if (clear_port_connection_feature.wIndex > hub_ports[hub]) {
    state[hub] = STATE_DONE;
    return true;
  }
  host_setup_transfer(hub, (uint8_t*)&clear_port_connection_feature,
                      sizeof(clear_port_connection_feature),
                      STATE_CLEAR_PORT_CONNECTION_FEATURE_DONE);
  return false;
}

static bool state_clear_port_connection_feature_done(uint8_t hub) {
  clear_port_connection_feature.wIndex++;
  delay_ms(hub, 5, STATE_CLEAR_PORT_CONNECTION_FEATURE);
  return false;
}

static bool state_done(uint8_t hub) {
  unlock_transaction(hub);
  delay_ms(hub, 5, STATE_READY);
  return false;
}

static bool state_ready(uint8_t hub) {
  hub;
  return false;
}

static bool state_in_recv(uint8_t hub) {
  if (usb_host->in)
    usb_host->in(hub, buffer, user_request_size - transaction_size);
  do_not_retry[hub] = false;
  unlock_transaction(hub);
  delay_us(hub, 250, STATE_READY);
  return false;
}

static bool state_out_done(uint8_t hub) {
  unlock_transaction(hub);
  delay_us(hub, 250, STATE_READY);
  return false;
}

static bool state_hid_get_report(uint8_t hub) {
  if (usb_host->hid_report && !do_not_retry[hub])
    usb_host->hid_report(hub, buffer);
  do_not_retry[hub] = false;
  unlock_transaction(hub);
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

  uint8_t pid = transaction_ep_pid >> 4;
  uint8_t token = USB_INT_ST & MASK_UIS_HRES;
  if (pid == USB_PID_IN && token != USB_PID_NAK && token != USB_PID_STALL) {
    uint16_t size = USB_RX_LEN;
    for (uint16_t i = 0; i < size; ++i)
      transaction_buffer[i] = rx_buffer[i];
#ifdef _USB_HOST_DBG_LOG
    usb_host_log_recv(transaction_ep_pid & 0x0f, transaction_ep_pid >> 4, size,
                      transaction_buffer);
#endif  // _USB_HOST_DBG_LOG
    transaction_buffer = &transaction_buffer[size];
    transaction_size -= size;
  }

  if (token == USB_PID_STALL) {
#ifdef _USB_HOST_DBG_LOG
    usb_host_log_stall();
#endif  // _USB_HOST_DBG_LOG
    // Try resetting, but may be no luck.
    state[hub] = STATE_CONNECT;
    return true;
  } else if (U_TOG_OK || token == USB_PID_DATA0 || token == USB_PID_DATA1 ||
             token == 0) {
    if (transaction_size &&
        USB_RX_LEN == ep_max_packet_size[hub][transaction_ep_pid & 0x0f]) {
      delay_us(hub, 250, STATE_TRANSACTION_CONT);
      return false;
    }

    // Succeeded.
    if (pid == USB_PID_SETUP) {
      // Proceed data stage.
      const struct usb_setup_req* req = (const struct usb_setup_req*)tx_buffer;
      if ((req->bRequestType & USB_REQ_DIR_MASK) == USB_REQ_DIR_IN) {
        pid = USB_PID_IN;
        if (req->wLength) {
          delay_us(hub, 250, STATE_TRANSACTION_IN);
          return false;
        }
      } else if ((req->bRequestType & USB_REQ_DIR_MASK) == USB_REQ_DIR_OUT) {
        pid = USB_PID_OUT;
        if (req->wLength) {
          delay_us(hub, 250, STATE_TRANSACTION_OUT);
          return false;
        }
      }
    }
    if (transaction_stage == 0) {
      // Proceed status stage.
      transaction_stage++;
      if (pid == USB_PID_IN) {
        delay_us(hub, 250, STATE_TRANSACTION_OUT);
        return false;
      } else if (pid == USB_PID_OUT) {
        delay_us(hub, 250, STATE_TRANSACTION_IN);
        return false;
      }
    }
    state[hub] = transaction_recv_state;
    do_not_retry[hub] = false;
    return true;
  } else if (token == USB_PID_NAK) {
#ifdef _USB_HOST_DBG_LOG
    usb_host_log_nak();
#endif  // _USB_HOST_DBG_LOG
    if (do_not_retry[hub] == true) {
      // Keeping `do_not_retry` means it fails with NAK.
      state[hub] = transaction_recv_state;
      return true;
    }
    delay_us(hub, 250, STATE_TRANSACTION_RETRY);
    return false;
  }

  Serial.printf("\ntransmit error: %x\n", USB_INT_ST);
  halt(hub);
  return false;
}

static bool state_transaction_in(uint8_t hub) {
  const struct usb_setup_req* req = (const struct usb_setup_req*)tx_buffer;
  const uint8_t ep = transaction_ep_pid & 0x0f;
  user_request_size = (transaction_stage == 1) ? 0 : req->wLength;
  uint8_t tog = (transaction_stage == 1) ? AUTO_TOGGLE : 0;
  host_in_transfer(hub, ep, user_request_size, transaction_recv_state, tog);
  return false;
}

static bool state_transaction_out(uint8_t hub) {
  const struct usb_setup_req* req = (const struct usb_setup_req*)tx_buffer;
  const uint8_t ep = transaction_ep_pid & 0x0f;
  uint16_t size = (transaction_stage == 1) ? 0 : req->wLength;
  uint8_t tog = (transaction_stage == 1) ? AUTO_TOGGLE : 0;
  host_out_transfer(hub, ep, buffer, size, transaction_recv_state, tog);
  return false;
}

static bool state_transaction_ack(uint8_t hub) {
  const uint8_t ep = transaction_ep_pid & 0x0f;
  host_ack_transfer(hub, ep, 0, 0, transaction_recv_state);
  return false;
}

static bool state_transaction_cont(uint8_t hub) {
  host_transact_cont(hub, AUTO_TOGGLE);
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
      unlock_transaction(hub);
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
    case STATE_SET_ADDRESS:
      return state_set_address(hub);
    case STATE_SET_ADDRESS_DONE:
      return state_set_address_done(hub);
    case STATE_GET_DEVICE_DESC:
      return state_get_device_desc(hub);
    case STATE_GET_DEVICE_DESC_RECV:
      return state_get_device_desc_recv(hub);
    case STATE_GET_STRING_DESC:
      return state_get_string_desc(hub);
    case STATE_GET_STRING_DESC_RECV:
      return state_get_string_desc_recv(hub);
    case STATE_GET_CONFIGURATION_DESC:
      return state_get_configuration_desc(hub);
    case STATE_GET_CONFIGURATION_DESC_RECV:
      return state_get_configuration_desc_recv(hub);
    case STATE_SET_CONFIGURATION:
      return state_set_configuration(hub);
    case STATE_SET_CONFIGURATION_DONE:
      return state_set_configuration_done(hub);
    case STATE_SET_FEATURE:
      return state_set_feature(hub);
    case STATE_EXTRA_SETUP:
      return state_extra_setup(hub);
    case STATE_GET_HID_REPORT_DESC:
      return state_get_hid_report_desc(hub);
    case STATE_GET_HID_REPORT_DESC_RECV:
      return state_get_hid_report_desc_recv(hub);
    case STATE_HID_SET_PROTOCOL:
      return state_hid_set_protocol(hub);
    case STATE_GET_HUB_DESC:
      return state_get_hub_desc(hub);
    case STATE_GET_HUB_DESC_RECV:
      return state_get_hub_desc_recv(hub);
    case STATE_SET_PORT_POWER_FEATURE:
      return state_set_port_power_feature(hub);
    case STATE_SET_PORT_POWER_FEATURE_DONE:
      return state_set_port_power_feature_done(hub);
    case STATE_CLEAR_PORT_CONNECTION_FEATURE:
      return state_clear_port_connection_feature(hub);
    case STATE_CLEAR_PORT_CONNECTION_FEATURE_DONE:
      return state_clear_port_connection_feature_done(hub);
    case STATE_DONE:
      return state_done(hub);
    case STATE_READY:
      return state_ready(hub);
    case STATE_HALT:
      return false;
    case STATE_IN_RECV:
      return state_in_recv(hub);
    case STATE_OUT_DONE:
      return state_out_done(hub);
    case STATE_HID_GET_REPORT:
      return state_hid_get_report(hub);
    case STATE_DELAY_US:
      return state_delay_us(hub);
    case STATE_DELAY_MS:
      return state_delay_ms(hub);
    case STATE_TRANSACTION:
      return state_transaction(hub);
    case STATE_TRANSACTION_IN:
      return state_transaction_in(hub);
    case STATE_TRANSACTION_OUT:
      return state_transaction_out(hub);
    case STATE_TRANSACTION_ACK:
      return state_transaction_ack(hub);
    case STATE_TRANSACTION_CONT:
      return state_transaction_cont(hub);
    case STATE_TRANSACTION_RETRY:
      return state_transaction_retry(hub);
    default:
      halt(hub);
  }
  return false;
}

#ifdef _IMPL_USB_HOST_LOG_SEND
void usb_host_log_send(uint8_t ep, uint8_t pid, uint8_t size, uint8_t* buffer) {
  Serial.printf("send ep: %d, pid: %d, size: %d; ", ep, pid, size);
  for (uint8_t i = 0; i < size; ++i)
    Serial.printf("%x,", buffer[i]);
  Serial.printf("\n");
}
#endif  // _IMPL_USB_HOST_SEND_LOG

#ifdef _IMPL_USB_HOST_LOG_RECV
void usb_host_log_recv(uint8_t ep, uint8_t pid, uint8_t size, uint8_t* buffer) {
  Serial.printf("recv ep: %d, pid: %d, size: %d; ", ep, pid, size);
  for (uint8_t i = 0; i < size; ++i)
    Serial.printf("%x,", buffer[i]);
  Serial.printf("\n");
}
#endif  // _IMPL_USB_HOST_RECV_LOG

#ifdef _IMPL_USB_HOST_LOG_STALL
void usb_host_log_stall(void) {
  Serial.println("stall");
}
#endif  // _IMPL_USB_HOST_STALL_LOG

#ifdef _IMPL_USB_HOST_LOG_NAK
void usb_host_log_nak(void) {
  Serial.println("nak");
}
#endif  // _IMPL_USB_HOST_LOG_NAK

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
  USB_INT_FG = 0xff;      // Clear interrupt flags
  if (host->flags & USE_HUB0)
    UHUB0_CTRL = 0x00;  // Enable HUB0
  if (host->flags & USE_HUB1)
    UHUB1_CTRL = 0x00;  // Enable HUB1
  USB_INT_EN = bUIE_TRANSFER | bUIE_DETECT;

  for (uint8_t i = 0; i < 2; ++i)
    state[i] = STATE_IDLE;

  timer3_tick_init();
}

void usb_host_poll(void) {
  if (usb_host->flags & USE_HUB0)
    while (fsm(0))
      ;
  if (usb_host->flags & USE_HUB1)
    while (fsm(1))
      ;
}

bool usb_host_ready(uint8_t hub) {
  return state[hub] == STATE_READY && !is_transaction_locked();
}

bool usb_host_idle(void) {
  return !is_transaction_locked();
}

bool usb_host_setup(uint8_t hub,
                    const struct usb_setup_req* req,
                    const uint8_t* data) {
  if (!usb_host_ready(hub) ||
      !lock_transaction(hub, hub_address[hub] ? hub_address[hub] : 1 + hub)) {
    return false;
  }
  bool dir_in = (req->bRequestType & USB_REQ_DIR_MASK) == USB_REQ_DIR_IN;
  if (!dir_in) {
    for (uint16_t i = 0; i < req->wLength; ++i)
      buffer[i] = data[i];
  }
  host_setup_transfer(hub, (uint8_t*)req, sizeof(struct usb_setup_req),
                      dir_in ? STATE_IN_RECV : STATE_DONE);
  return true;
}

bool usb_host_in(uint8_t hub, uint8_t ep, uint8_t size) {
  if (!usb_host_ready(hub) ||
      !lock_transaction(hub, hub_address[hub] ? hub_address[hub] : 1 + hub)) {
    return false;
  }
  transaction_stage = 2;
  // Do not retry as hid returns NAK if the report isn't changed in idle state.
  // This flag keeps true if the request fails with NAK.
  do_not_retry[hub] = true;
  user_request_size = size;
  host_in_transfer(hub, ep, size, STATE_IN_RECV, 0);
  return true;
}

bool usb_host_in_data0(uint8_t hub, uint8_t ep, uint8_t size) {
  if (!usb_host_ready(hub) ||
      !lock_transaction(hub, hub_address[hub] ? hub_address[hub] : 1 + hub)) {
    return false;
  }
  transaction_stage = 0;
  // Do not retry as hid returns NAK if the report isn't changed in idle state.
  // This flag keeps true if the request fails with NAK.
  do_not_retry[hub] = true;
  user_request_size = size;
  host_in_transfer(hub, ep, size, STATE_IN_RECV, 0);
  return true;
}

bool usb_host_out(uint8_t hub, uint8_t ep, uint8_t* data, uint8_t size) {
  if (!usb_host_ready(hub) ||
      !lock_transaction(hub, hub_address[hub] ? hub_address[hub] : 1 + hub)) {
    return false;
  }
  transaction_stage = 2;
  host_out_transfer(hub, ep, data, size, STATE_OUT_DONE, 0);
  return false;
}

bool usb_host_hid_get_report(uint8_t hub, uint8_t id, uint8_t size) {
  if (!is_hid[hub])
    return false;
  if (!usb_host_ready(hub) ||
      !lock_transaction(hub, hub_address[hub] ? hub_address[hub] : 1 + hub)) {
    return false;
  }
  hid_get_report.wValue = (1 << 8) | id;
  hid_get_report.wIndex = hid_interface_number[hub];
  hid_get_report.wLength = size;
  // Do not retry as hid returns NAK if the report isn't changed in idle state.
  // This flag keeps true if the request fails with NAK.
  do_not_retry[hub] = true;
  host_setup_transfer(hub, (uint8_t*)&hid_get_report, sizeof(hid_get_report),
                      STATE_HID_GET_REPORT);
  return true;
}

void usb_host_hub_switch(uint8_t hub, uint8_t address) {
  hub_address[hub] = address;
  state[hub] = STATE_SET_ADDRESS;
}