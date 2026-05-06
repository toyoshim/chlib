// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// BLE HID keyboard demo: pairs with a host (Just Works or Passkey Entry),
// then types 'a' once per second after notifications are enabled.

#include "ch559.h"
#include "serial.h"
#include "timer3.h"
#include "usb/ble/ble_hid_peripheral.h"
#include "usb/usb_host.h"

enum {
  APPEARANCE_KEYBOARD = 0x03c1,
  BLE_PASSKEY         = 738291,
  HID_REPORT_LEN      = 8,
  HID_KEYCODE_A       = 0x04,
};

// HID Boot Keyboard Report Map (HID 1.11 sec. B.1).
static const uint8_t HID_REPORT_MAP[] = {
    0x05, 0x01,  // usage page (desktop)
    0x09, 0x06,  // usage (keyboard)
    0xa1, 0x01,  // collection (application)
    0x05, 0x07,  // usage page (key codes)
    0x19, 0xe0,  // usage minimum (224 / left control)
    0x29, 0xe7,  // usage maximum (231 / right gui)
    0x15, 0x00,  // logical minimum (0)
    0x25, 0x01,  // logical maximum (1)
    0x75, 0x01,  // report size (1)
    0x95, 0x08,  // report count (8)
    0x81, 0x02,  // input (modifiers byte)
    0x95, 0x01,  // report count (1)
    0x75, 0x08,  // report size (8)
    0x81, 0x03,  // input constant (reserved byte)
    0x95, 0x05,  // report count (5)
    0x75, 0x01,  // report size (1)
    0x05, 0x08,  // usage page (LEDs)
    0x19, 0x01,  // usage minimum (num lock)
    0x29, 0x05,  // usage maximum (kana)
    0x91, 0x02,  // output (LED bitmap)
    0x95, 0x01,  // report count (1)
    0x75, 0x03,  // report size (3)
    0x91, 0x03,  // output constant (LED padding)
    0x95, 0x06,  // report count (6)
    0x75, 0x08,  // report size (8)
    0x15, 0x00,  // logical minimum (0)
    0x25, 0x65,  // logical maximum (101)
    0x05, 0x07,  // usage page (key codes)
    0x19, 0x00,  // usage minimum (0)
    0x29, 0x65,  // usage maximum (101)
    0x81, 0x00,  // input (key array)
    0xc0,        // end collection
};

static const uint8_t REPORT_PRESS_A[HID_REPORT_LEN] = {
    0, 0, HID_KEYCODE_A, 0, 0, 0, 0, 0,
};
static const uint8_t REPORT_RELEASE[HID_REPORT_LEN] = {0};

static bool key_down;
static uint16_t last_release_sec;

static void on_ready(void) {
  Serial.println("advertising");
}

static void on_connected(void) {
  Serial.println("connected");
}

static void on_disconnected(void) {
  Serial.println("disconnected, re-advertising");
}

static void on_notify_enabled(bool on) {
  if (on) {
    key_down = false;
    last_release_sec = timer3_tick_sec() - 1;
  }
}

static uint8_t on_battery_level(void) {
  // No real battery on a USB-powered CH559 demo; report a fixed level so
  // the host shows a plausible value.
  return 100;
}

static void on_sent(void) {
  if (key_down) {
    if (ble_hid_peripheral_send_report(REPORT_RELEASE, HID_REPORT_LEN)) {
      key_down = false;
      last_release_sec = timer3_tick_sec();
    }
  }
}

void main(void) {
  initialize();
  Serial.println("BLE Example: chlib example");

  static const struct ble_hid_peripheral hid = {
      .local_name = "chlib example",
      .appearance = APPEARANCE_KEYBOARD,
      .manufacturer = "chlib",
      .model = "ble example",
      .pnp_id = {BLE_VID_SOURCE_USB_IF, 0xffff, 0x0001, 0x0100},
      .report_map = HID_REPORT_MAP,
      .report_map_len = sizeof(HID_REPORT_MAP),
      .report_len = HID_REPORT_LEN,
      .passkey = BLE_PASSKEY,
      .battery_level = on_battery_level,
      .ready = on_ready,
      .connected = on_connected,
      .disconnected = on_disconnected,
      .notify_enabled = on_notify_enabled,
      .sent = on_sent,
  };
  // HUB0 is left for the PC (flashing / serial); the dongle goes on HUB1.
  ble_hid_peripheral_init(&hid, USE_HUB1);
  // TODO: the library sometimes fails to recognize a dongle plugged in at
  // power-on; force a bus reset as a workaround until that's fixed.
  usb_host_reset();

  for (;;) {
    ble_hid_peripheral_poll();
    if (!key_down && timer3_tick_sec() != last_release_sec) {
      if (ble_hid_peripheral_send_report(REPORT_PRESS_A, HID_REPORT_LEN)) {
        key_down = true;
      }
    }
  }
}
