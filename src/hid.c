// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "hid.h"

#include <string.h>

#include "ch559.h"
#if !defined(_HID_NO_PS3)
#include "hid_dualshock3.h"
#endif
#if !defined(_HID_NO_GUNCON3)
#include "hid_guncon3.h"
#endif
#include "hid_internal.h"
#if !defined(_HID_NO_KEYBOARD)
#include "hid_keyboard.h"
#endif
#if !defined(_HID_NO_MOUSE)
#include "hid_mouse.h"
#endif
#if !defined(_HID_NO_SWITCH)
#include "hid_switch.h"
#endif
#if !defined(_HID_NO_XBOX)
#include "hid_xbox.h"
#endif
#include "serial.h"
#include "usb.h"

// #define _DBG_DESC
// #define _DBG_HID_REPORT_DESC
// #define _DBG_HID_REPORT_DESC_DUMP

static struct hid* hid;
static struct usb_host host;
static struct hid_info hid_info[2];
static struct usb_info usb_info[2];

static void do_nothing(void) {}

static void disconnected(uint8_t hub) {
  hid_info[hub].state = HID_STATE_DISCONNECTED;
  hid_info[hub].report_size = 0;
  if (!hid->report)
    return;
  hid->report(hub, &hid_info[hub], 0, 0);
}

static void check_device_desc(uint8_t hub, const uint8_t* data) {
  hid_info[hub].report_desc_size = 0;
  hid_info[hub].state = HID_STATE_CONNECTED;
  hid_info[hub].type = HID_TYPE_UNKNOWN;
  const struct usb_desc_device* desc = (const struct usb_desc_device*)data;

#ifdef _DBG_DESC
  Serial.printf("device vendor: %x%x\n", desc->idVendor >> 8,
                desc->idVendor & 0xff);
  Serial.printf("device product: %x%x\n", desc->idProduct >> 8,
                desc->idProduct & 0xff);
  Serial.printf("device class: %x\n", desc->bDeviceClass);
  Serial.printf("device subclass: %x\n", desc->bDeviceSubClass);
  Serial.printf("device protocol: %x\n", desc->bDeviceProtocol);
#endif

  memset(&usb_info[hub], 0, sizeof(struct usb_info));
  usb_info[hub].tick = timer3_tick_raw();
  usb_info[hub].class = desc->bDeviceClass;
  usb_info[hub].vid = desc->idVendor;
  usb_info[hub].pid = desc->idProduct;

  if (desc->idVendor == 0x17a7 && desc->idProduct == 0x0005) {
    usb_info[hub].wait = 750;
  }

  if (false ||
#if !defined(_HID_NO_KEYBOARD)
      hid_keyboard_check_device_desc(&hid_info[hub], desc) ||
#endif
#if !defined(_HID_NO_MOUSE)
      hid_mouse_check_device_desc(&hid_info[hub], desc) ||
#endif
#if !defined(_HID_NO_XBOX)
      hid_xbox_check_device_desc(&hid_info[hub], desc) ||
#endif
#if !defined(_HID_NO_SWITCH)
      hid_switch_check_device_desc(&hid_info[hub], &usb_info[hub], desc) ||
#endif
#if !defined(_HID_NO_GUNCON3)
      hid_guncon3_check_device_desc(&hid_info[hub], &usb_info[hub], desc) ||
#endif
#if !defined(_HID_NO_DUALSHOCK3)
      hid_dualshock3_check_device_desc(&hid_info[hub], &usb_info[hub], desc) ||
#endif
      false) {
    return;
  }
}

static uint8_t check_configuration_desc(uint8_t hub, const uint8_t* data) {
  const struct usb_desc_configuration* desc =
      (const struct usb_desc_configuration*)data;
  struct usb_desc_head* head = (struct usb_desc_head*)data;
  uint8_t class = usb_info[hub].class;
  uint8_t target_interface = 0xff;
  for (uint8_t i = head->bLength; i < desc->wTotalLength; i += head->bLength) {
    head = (struct usb_desc_head*)(data + i);
    if (target_interface != 0xff &&
        head->bDescriptorType == USB_DESC_INTERFACE) {
      break;
    }
    switch (head->bDescriptorType) {
      case USB_DESC_INTERFACE: {
        const struct usb_desc_interface* intf =
            (const struct usb_desc_interface*)(data + i);
#ifdef _DBG_DESC
        Serial.printf("interface class: %x\n", intf->bInterfaceClass);
        Serial.printf("interface subclass: %x\n", intf->bInterfaceSubClass);
        Serial.printf("interface protocol: %x\n", intf->bInterfaceProtocol);
#endif  // _DBG_DESC
        if (usb_info[hub].class == 0) {
          class = intf->bInterfaceClass;
        }
        if (
#if !defined(_HID_NO_KEYBOARD)
            hid_keyboard_check_interface_desc(&hid_info[hub], intf) ||
#endif
#if !defined(_HID_NO_MOUSE)
            hid_mouse_check_interface_desc(&hid_info[hub], &usb_info[hub],
                                           intf) ||
#endif
#if !defined(_HID_NO_XBOX)
            hid_xbox_check_interface_desc(&hid_info[hub], intf) ||
#endif
#if !defined(_HID_NO_GUNCON3)
            hid_guncon3_check_interface_desc(&hid_info[hub], &usb_info[hub]) ||
#endif
            intf->bInterfaceClass == USB_CLASS_HID) {
          target_interface = intf->bInterfaceNumber;
        }
        break;
      }
      case USB_DESC_HID: {
        const struct usb_desc_hid* hid = (const struct usb_desc_hid*)(data + i);
        hid_info[hub].report_desc_size = hid->wDescriptorLength;
        break;
      }
      case USB_DESC_ENDPOINT: {
        if (hid_info[hub].type == HID_TYPE_UNKNOWN && class != USB_CLASS_HID) {
          break;
        }
        const struct usb_desc_endpoint* ep =
            (const struct usb_desc_endpoint*)(data + i);
        if (ep->bEndpointAddress >= 128 && (ep->bmAttributes & 3) == 3) {
          // interrupt input.
          usb_info[hub].ep_in = ep->bEndpointAddress & 0x0f;
          usb_info[hub].ep_max_packet_size = ep->wMaxPacketSize;
#ifdef _DBG_DESC
          Serial.printf("ep in: %d\n", usb_info[hub].ep_in);
#endif
        } else if (ep->bEndpointAddress < 128 && (ep->bmAttributes & 3) == 3) {
          // interrupt output.
          usb_info[hub].ep_out = ep->bEndpointAddress & 0x0f;
#ifdef _DBG_DESC
          Serial.printf("ep out: %d\n", usb_info[hub].ep_out);
#endif
        }
        break;
      }
    }
  }
#ifdef _DBG_DESC
  Serial.printf("report_desc_size: %d, ep_in: %d\n",
                hid_info[hub].report_desc_size, usb_info[hub].ep_in);
#endif
  if (hid_info[hub].report_desc_size && usb_info[hub].ep_in) {
    hid_info[hub].state = HID_STATE_NOT_READY;
  }

  if (false ||
#if !defined(_HID_NO_KEYBOARD)
      hid_keyboard_initialize(&hid_info[hub]) ||
#endif
#if !defined(_HID_NO_GUNCON3)
      hid_guncon3_initialize(&hid_info[hub], &usb_info[hub]) ||
#endif
#if !defined(_HID_NO_XBOX)
      hid_xbox_initialize(&hid_info[hub], &usb_info[hub]) ||
#endif
      false) {
    if (hid->detected) {
      hid->detected();
    }
  }
  usb_info[hub].interface = target_interface;
  return target_interface;
}

#ifdef _DBG_HID_REPORT_DESC
#define REPORT(s) Serial.printf("Report %s: %x\n", s, data[i])
#define REPORT0(s) Serial.println(s " (0)")
#define REPORT1(s) Serial.printf(s " (1): %x\n", data[i + 1])
#define REPORT2(s) Serial.printf(s " (2): %x%x\n", data[i + 2], data[i + 1])
#else
#define REPORT(s)
#define REPORT0(s)
#define REPORT1(s)
#define REPORT2(s)
#pragma disable_warning 110
#endif

static void check_hid_report_desc(uint8_t hub, const uint8_t* data) {
  if (hid_info[hub].state != HID_STATE_NOT_READY) {
    return;
  }
#ifdef _DBG_HID_REPORT_DESC_DUMP
  {
    for (uint16_t i = 0; i < hid_info[hub].report_desc_size; ++i)
      Serial.printf("0x%x, ", data[i]);
  }
#endif
  const uint16_t size = hid_info[hub].report_desc_size;
  hid_info[hub].report_size = 0;
  for (uint8_t button = 0; button < 6; ++button) {
    hid_info[hub].axis[button] = 0xffff;
  }
  hid_info[hub].hat = 0xffff;
  for (uint8_t button = 0; button < 4; ++button) {
    hid_info[hub].dpad[button] = 0xffff;
  }
  for (uint8_t button = 0; button < 13; ++button) {
    hid_info[hub].button[button] = 0xffff;
  }
  hid_info[hub].report_id = 0;
  uint8_t report_size = 0;
  uint8_t report_count = 0;
  uint16_t usage_page = 0;
  uint8_t usage_index = 0;
  uint32_t usages[12];
  uint8_t button_index = 0;
  for (usage_index = 0; usage_index < 12; ++usage_index) {
    usages[usage_index] = 0;
  }
  usage_index = 0;
  uint8_t analog_index = 0;
  for (uint16_t i = 0; i < size;) {
    // Long items are not supported
    if (data[i] == 0xfe) {
      if ((i + 1) < size) {
        i += data[i + 1] + 3;
        continue;
      } else {
        break;  // for
      }
    }
    // Short items
    uint8_t b_size = data[i] & 3;
    if (b_size == 0) {  // 0 byte items
      switch (data[i]) {
        case 0xc0:
          REPORT0("M:End Collection");
          for (usage_index = 0; usage_index < 12; ++usage_index) {
            usages[usage_index] = 0;
          }
          usage_index = 0;
          break;
        default:  // ignore
          REPORT("Skip 0");
          break;
      }
      i++;
    } else if (b_size == 1) {  // 1 bytes items
      if ((i + 1) >= size) {
        break;  // for
      }
      switch (data[i]) {
        case 0x05:
          REPORT1("G:Usage Page");
          usage_page = data[i + 1];
          break;
        case 0x09:
          REPORT1("L:Usage");
          if (usage_index < 12) {
            usages[usage_index++] = ((uint32_t)usage_page << 16) | data[i + 1];
          }
          break;
        case 0x15:
          REPORT1("G:Logical Minimum");
          break;
        case 0x19:
          REPORT1("L:Usage Minimum");
          break;
        case 0x25:
          REPORT1("G:Logical Maximum");
          break;
        case 0x29:
          REPORT1("L:Usage Maxmum");
          break;
        case 0x35:
          REPORT1("G:Physical Minimum");
          break;
        case 0x65:
          REPORT1("G:Unit");
          break;
        case 0x75:
          REPORT1("G:Report Size");
          report_size = data[i + 1];
          break;
        case 0x81:
          REPORT1("M:Input");
          if (data[i + 1] & 1) {
            // Skip constant
          } else if (usages[0] == 0x00010039 && report_size == 4 &&
                     (data[i + 1] & 1) == 0) {  // Hat switch
            hid_info[hub].hat = hid_info[hub].report_size;
          } else if (usages[0] == 0xff000020 && report_size == 6) {
            // PS4 counter
            hid_info[hub].type = HID_TYPE_PS4;
          } else if (report_size == 1) {  // Buttons
            for (uint8_t i = 0; i < report_count && button_index < 13; ++i) {
              hid_info[hub].button[button_index++] =
                  hid_info[hub].report_size + i;
            }
          } else if ((data[i + 1] & 1) == 0) {  // Analog buttons
            for (uint8_t i = 0; i < report_count && analog_index < 6; ++i) {
              if (usages[i] == 0x00010030) {
                analog_index = 0;
              } else if (usages[i] == 0x00010031) {
                analog_index = 1;
              } else if (usages[i] == 0x00010032) {
                analog_index = 2;
              } else if (usages[i] == 0x00010035) {
                analog_index = 3;
              }
              hid_info[hub].axis_size[analog_index] = report_size;
              hid_info[hub].axis_shift[analog_index] = 0;
              hid_info[hub].axis_sign[analog_index] = false;
              hid_info[hub].axis_polarity[analog_index] = false;
              hid_info[hub].axis[analog_index++] =
                  hid_info[hub].report_size + report_size * i;
              while (analog_index < 6 &&
                     hid_info[hub].axis[analog_index] != 0xffff) {
                analog_index++;
              }
            }
          }
          usage_index = 0;
          hid_info[hub].report_size += report_size * report_count;
          break;
        case 0x85:
          if (hid_info[hub].report_size &&
              (hid_info[hub].hat != 0xffff || hid_info[hub].dpad[3] != 0xffff ||
               hid_info[hub].axis[1] != 0xffff) &&
              hid_info[hub].button[1] != 0xffff) {
            goto quit;
          }
          hid_info[hub].report_size = 0;
          for (uint8_t button = 0; button < 6; ++button) {
            hid_info[hub].axis[button] = 0xffff;
          }
          hid_info[hub].hat = 0xffff;
          for (uint8_t button = 0; button < 4; ++button) {
            hid_info[hub].dpad[button] = 0xffff;
          }
          for (uint8_t button = 0; button < 13; ++button) {
            hid_info[hub].button[button] = 0xffff;
          }
          for (uint8_t button = 0; button < 12; ++button) {
            usages[button] = 0;
          }
          usage_index = 0;
          button_index = 0;
          analog_index = 0;
          REPORT1("G:Report ID");
          hid_info[hub].report_id = data[i + 1];
          break;
        case 0x95:
          REPORT1("G:Report Count");
          report_count = data[i + 1];
          break;
        case 0xa1:
          REPORT1("M:Collection");
          for (usage_index = 0; usage_index < 12; ++usage_index) {
            usages[usage_index] = 0;
          }
          usage_index = 0;
          break;
        default:  // not supported
          REPORT("Skip 1");
          break;
      }
      i += 2;
    } else if (b_size == 2) {
      // 2 bytes items
      if ((i + 2) >= size) {
        break;
      }
      switch (data[i]) {
        case 0x06:
          REPORT2("G:Usage Page");
          usage_page = (data[i + 2] << 8) | data[i + 1];
          break;
        case 0x0a:
          REPORT2("L:Usage");
          if (usage_index < 12) {
            usages[usage_index++] =
                ((uint32_t)usage_page << 16) | (data[i + 2] << 8) | data[i + 1];
          }
          break;
        case 0x16:
          REPORT2("G:Logical Minimum");
          break;
        case 0x26:
          REPORT2("G:Logical Maximum");
          break;
        case 0x36:
          REPORT2("G:Physical Minimum");
          break;
        case 0x46:
          REPORT2("G:Physical Maximum");
          break;
        default:  // not supported
          REPORT("Skip 2");
          break;
      }
      i += 3;
    } else {  // 4 bytes items
      i += 5;
    }
  }
quit:
#ifdef _DBG_HID_REPORT_DESC
  Serial.printf("Report Size for ID (%d): %d-bits (%d-Bytes)\n",
                hid_info[hub].report_id, hid_info[hub].report_size,
                hid_info[hub].report_size / 8);
  for (uint8_t i = 0; i < 4; ++i) {
    Serial.printf("axis %d: %d, %d\n", i, hid_info[hub].axis[i],
                  hid_info[hub].axis_size[i]);
  }
  Serial.printf("hat: %d\n", hid_info[hub].hat);
  for (uint8_t i = 0; i < 13; ++i) {
    Serial.printf("button %d: %d\n", i, hid_info[hub].button[i]);
  }
#endif
  if (hid_info[hub].type == HID_TYPE_UNKNOWN) {
    if (hid_info[hub].report_size && hid_info[hub].button[12] != 0xfff &&
        ((hid_info[hub].axis[0] != 0xffff && hid_info[hub].axis[1] != 0xffff) ||
         hid_info[hub].hat != 0xffff ||
         (hid_info[hub].dpad[0] != 0xffff && hid_info[hub].dpad[1] != 0xffff &&
          hid_info[hub].dpad[2] != 0xffff &&
          hid_info[hub].dpad[3] != 0xffff))) {
      hid_info[hub].type = HID_TYPE_GENERIC;
    }
  }
  hid_info[hub].state = HID_STATE_READY;
#if !defined(_HID_NO_SWITCH)
  if (hid_info[hub].type == HID_TYPE_SWITCH) {
    hid_switch_initialize(&hid_info[hub]);
  }
#endif
#if !defined(_HID_NO_PS3)
  if (hid_info[hub].type == HID_TYPE_PS3) {
    hid_dualshock3_initialize(&hid_info[hub]);
  }
#endif
  if (hid_info[hub].type != HID_TYPE_UNKNOWN) {
    if (hid->detected) {
      hid->detected();
    }
  }

  // Device specific fix-up.
  if (usb_info[hub].vid == 0x046d && usb_info[hub].pid == 0xc260) {
    // G29 Driving Force Racing Wheel [PS4]
    hid_info[hub].axis[0] = 336;
    hid_info[hub].axis_size[0] = 16;
    hid_info[hub].axis[1] = 352;
    hid_info[hub].axis_size[1] = 16;
    hid_info[hub].axis_polarity[1] = true;
    hid_info[hub].axis[2] = 368;
    hid_info[hub].axis_size[2] = 16;
    hid_info[hub].axis_polarity[2] = true;
    hid_info[hub].axis[3] = 384;
    hid_info[hub].axis_size[3] = 16;
    hid_info[hub].axis_polarity[3] = true;
  } else if (usb_info[hub].vid == 0x046d && usb_info[hub].pid == 0xc294) {
    // G29 Driving Force Racing Wheel [PS3]
    hid_info[hub].axis[1] = 136;
    hid_info[hub].axis[2] = 144;
    hid_info[hub].type = HID_TYPE_PS4;
  } else if (usb_info[hub].vid == 0x0f0d &&
             (usb_info[hub].pid == 0x00a7 || usb_info[hub].pid == 0x00a8)) {
    // HOLI Flight Stick [PS4] / [PS3]
    hid_info[hub].state = HID_STATE_SET_IDLE;
    usb_info[hub].get_report_value = 0x0303;
    usb_info[hub].get_report_length = 0x30;
  }
}

static void hid_report(uint8_t hub, uint8_t* data, uint16_t size) {
#if !defined(_HID_NO_PS3)
  if (hid_dualshock3_report(&hid_info[hub], &usb_info[hub], data, size)) {
    return;
  }
#endif
#if !defined(_HID_NO_GUNCON3)
  if (hid_guncon3_report(&hid_info[hub], &usb_info[hub], data, size)) {
    return;
  }
#endif
#if !defined(_HID_NO_XBOX)
  if (hid_xbox_report(&hid_info[hub], data, size)) {
    return;
  }
#endif
#if !defined(_HID_NO_SWITCH)
  if (hid_switch_report(hub, &hid_info[hub], &usb_info[hub], data, size)) {
    return;
  }
#endif
  if (hid->report && size) {
    hid->report(hub, &hid_info[hub], data, size);
  }
  if (usb_info[hub].wait) {
    usb_info[hub].tick = timer3_tick_raw();
  }
}

void hid_init(struct hid* new_hid) {
  hid = new_hid;
  if (hid->get_flags) {
    host.flags = hid->get_flags();
  } else {
    host.flags = USE_HUB1 | USE_HUB0;
  }
  if (!hid->detected)
    hid->detected = do_nothing;

  host.disconnected = disconnected;
  host.check_device_desc = check_device_desc;
  host.check_string_desc = 0;
  host.check_configuration_desc = check_configuration_desc;
  host.check_hid_report_desc = check_hid_report_desc;
  host.in = hid_report;
  host.hid_report = hid_report;
  usb_host_init(&host);
}

struct hid_info* hid_get_info(uint8_t hub) {
  return &hid_info[hub];
}

void hid_poll(void) {
  static uint8_t next_hub = 0;
  usb_host_poll();
  if (!usb_host_idle()) {
    return;
  }
  uint8_t hub = next_hub;
  next_hub = (next_hub + 1) & 1;
  uint16_t wait = usb_info[hub].wait;
  if (wait) {
    uint16_t begin = usb_info[hub].tick;
    if (timer3_tick_raw_between(begin, begin + wait)) {
      return;
    }
  }
  if (!usb_host_ready(hub)) {
    return;
  }
  if (hid_info[hub].state == HID_STATE_READY) {
    switch (hid_info[hub].type) {
#if !defined(_HID_NO_GUNCON3)
      case HID_TYPE_ZAPPER:
        hid_guncon3_poll(hub, &usb_info[hub]);
        break;
#endif
#if !defined(_HID_NO_PS3)
      case HID_TYPE_PS3:
        hid_dualshock3_poll(hub, &hid_info[hub], &usb_info[hub]);
        break;
#endif
#if !defined(_HID_NO_XBOX)
      case HID_TYPE_XBOX_360:
        hid_xbox_360_poll(hub, &usb_info[hub]);
        break;
      case HID_TYPE_XBOX_ONE:
        hid_xbox_one_poll(hub, &usb_info[hub]);
        break;
#endif
#if !defined(_HID_NO_SWITCH)
      case HID_TYPE_SWITCH:
        hid_switch_poll(hub, &usb_info[hub]);
        break;
#endif
      default: {
        uint16_t size = hid_info[hub].report_size / 8;
        if (hid_info[hub].report_id) {
          size++;
        }
        usb_host_in(hub, usb_info[hub].ep_in, size);
        break;
      }
    } /* switch */
  } else if (hid_info[hub].state == HID_STATE_SET_IDLE) {
    static struct usb_setup_req set_idle = {
        USB_REQ_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_INTERFACE,
        USB_HID_SET_IDLE, 0, 0, 0};
    set_idle.wIndex = usb_info[hub].interface;
    usb_host_setup(hub, &set_idle, 0);
    hid_info[hub].state = HID_STATE_GET_REPORT;
  } else if (hid_info[hub].state == HID_STATE_GET_REPORT) {
    static struct usb_setup_req get_report = {
        USB_REQ_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_RECPT_INTERFACE,
        USB_HID_GET_REPORT, 0, 0, 0};
    get_report.wIndex = usb_info[hub].interface;
    get_report.wValue = usb_info[hub].get_report_value;
    get_report.wLength = usb_info[hub].get_report_length;
    usb_host_setup(hub, &get_report, 0);
    hid_info[hub].state = HID_STATE_READY;
  }
}