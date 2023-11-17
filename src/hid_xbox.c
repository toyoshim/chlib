// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "hid_xbox.h"

#include "hid.h"
#include "hid_internal.h"
#include "usb.h"

// state
enum {
  CONNECTED,
  INITIALIZED,
  STARTED,
};

static bool check(struct hid_info* hid_info,
                  uint8_t any_class,
                  uint8_t any_subclass,
                  uint8_t any_protocol) {
  if (any_class == 0xff && any_subclass == 0x5d && any_protocol == 0x01) {
    // Might be a Xbox 360 compatible controller.
    hid_info->type = HID_TYPE_XBOX_360;
    hid_info->report_desc_size = 1;
    return true;
  }
  if (any_class == 0xff && any_subclass == 0x47 && any_protocol == 0xd0) {
    // Might be a Xbox One compatible controller.
    hid_info->type = HID_TYPE_XBOX_ONE;
    hid_info->report_desc_size = 1;
    return true;
  }
  return false;
}

bool hid_xbox_check_device_desc(struct hid_info* hid_info,
                                const struct usb_desc_device* desc) {
  if (desc->idVendor == 0x045e) {
    // Microsoft Xbox 360 / ONE official controllers.
    if (desc->idProduct == 0x028e) {
      hid_info->type = HID_TYPE_XBOX_360;
      hid_info->report_desc_size = 1;
      return true;
    }
    if (desc->idProduct == 0x02d1 || desc->idProduct == 0x02dd ||
        desc->idProduct == 0x02e3 || desc->idProduct == 0x02ea ||
        desc->idProduct == 0x0b00 || desc->idProduct == 0x0b0a ||
        desc->idProduct == 0x0b12) {
      hid_info->type = HID_TYPE_XBOX_ONE;
      hid_info->report_desc_size = 1;
      return true;
    }
  }
  return check(hid_info, desc->bDeviceClass, desc->bDeviceSubClass,
               desc->bDeviceProtocol);
}

bool hid_xbox_check_interface_desc(struct hid_info* hid_info,
                                   const struct usb_desc_interface* intf) {
  return check(hid_info, intf->bInterfaceClass, intf->bInterfaceSubClass,
               intf->bInterfaceProtocol);
}

bool hid_xbox_initialize(struct hid_info* hid_info, struct usb_info* usb_info) {
  if (hid_info->type != HID_TYPE_XBOX_360 &&
      hid_info->type != HID_TYPE_XBOX_ONE) {
    return false;
  }

  hid_info->state = HID_STATE_READY;
  usb_info->state = CONNECTED;
  usb_info->cmd_count = 0;

  if (hid_info->type == HID_TYPE_XBOX_360) {
    // https://github.com/xboxdrv/xboxdrv/blob/stable/PROTOCOL
    hid_info->report_size = 20 * 8;
    hid_info->axis[0] = 6 * 8;
    hid_info->axis_size[0] = 16;
    hid_info->axis_shift[0] = 0;
    hid_info->axis_sign[0] = true;
    hid_info->axis_polarity[0] = false;
    hid_info->axis[1] = 8 * 8;
    hid_info->axis_size[1] = 16;
    hid_info->axis_shift[1] = 0;
    hid_info->axis_sign[1] = true;
    hid_info->axis_polarity[1] = true;
    hid_info->axis[2] = 10 * 8;
    hid_info->axis_size[2] = 16;
    hid_info->axis_shift[2] = 0;
    hid_info->axis_sign[2] = true;
    hid_info->axis_polarity[2] = false;
    hid_info->axis[3] = 12 * 8;
    hid_info->axis_size[3] = 16;
    hid_info->axis_shift[3] = 0;
    hid_info->axis_sign[3] = true;
    hid_info->axis_polarity[3] = true;
    hid_info->axis[4] = 4 * 8;
    hid_info->axis_size[4] = 8;
    hid_info->axis_shift[4] = 0;
    hid_info->axis_sign[4] = false;
    hid_info->axis_polarity[4] = false;
    hid_info->axis[5] = 5 * 8;
    hid_info->axis_size[5] = 8;
    hid_info->axis_shift[5] = 0;
    hid_info->axis_sign[5] = false;
    hid_info->axis_polarity[5] = false;
    hid_info->hat = 0xffff;
    hid_info->dpad[0] = 16 + 0;
    hid_info->dpad[1] = 16 + 1;
    hid_info->dpad[2] = 16 + 2;
    hid_info->dpad[3] = 16 + 3;
    hid_info->button[0] = 24 + 6;
    hid_info->button[1] = 24 + 4;
    hid_info->button[2] = 24 + 5;
    hid_info->button[3] = 24 + 7;
    hid_info->button[4] = 24 + 0;
    hid_info->button[5] = 24 + 1;
    hid_info->button[6] = 39;
    hid_info->button[7] = 47;
    hid_info->button[8] = 16 + 5;
    hid_info->button[9] = 16 + 4;
    hid_info->button[10] = 16 + 6;
    hid_info->button[11] = 16 + 7;
    hid_info->report_id = 0;
  } else {
    // https://github.com/quantus/xbox-one-controller-protocol
    hid_info->report_size = 18 * 8;
    hid_info->axis[0] = 10 * 8;
    hid_info->axis_size[0] = 16;
    hid_info->axis_shift[0] = 0;
    hid_info->axis_sign[0] = true;
    hid_info->axis_polarity[0] = false;
    hid_info->axis[1] = 12 * 8;
    hid_info->axis_size[1] = 16;
    hid_info->axis_shift[1] = 0;
    hid_info->axis_sign[1] = true;
    hid_info->axis_polarity[1] = true;
    hid_info->axis[2] = 14 * 8;
    hid_info->axis_size[2] = 16;
    hid_info->axis_shift[2] = 0;
    hid_info->axis_sign[2] = true;
    hid_info->axis_polarity[2] = false;
    hid_info->axis[3] = 16 * 8;
    hid_info->axis_size[3] = 16;
    hid_info->axis_shift[3] = 0;
    hid_info->axis_sign[3] = true;
    hid_info->axis_polarity[3] = true;
    hid_info->axis[4] = 6 * 8;
    hid_info->axis_size[4] = 16;
    hid_info->axis_shift[4] = 5;
    hid_info->axis_sign[4] = false;
    hid_info->axis_polarity[4] = false;
    hid_info->axis[5] = 8 * 8;
    hid_info->axis_size[5] = 16;
    hid_info->axis_shift[5] = 5;
    hid_info->axis_sign[5] = false;
    hid_info->axis_polarity[5] = false;
    hid_info->hat = 0xffff;
    hid_info->dpad[0] = 40 + 0;
    hid_info->dpad[1] = 40 + 1;
    hid_info->dpad[2] = 40 + 2;
    hid_info->dpad[3] = 40 + 3;
    hid_info->button[0] = 32 + 6;
    hid_info->button[1] = 32 + 4;
    hid_info->button[2] = 32 + 5;
    hid_info->button[3] = 32 + 7;
    hid_info->button[4] = 40 + 4;
    hid_info->button[5] = 40 + 5;
    hid_info->button[6] = 57;
    hid_info->button[7] = 73;
    hid_info->button[8] = 32 + 3;
    hid_info->button[9] = 32 + 2;
    hid_info->button[10] = 40 + 6;
    hid_info->button[11] = 40 + 7;
    hid_info->report_id = 0;
  }
  return true;
}

bool hid_xbox_report(struct hid_info* hid_info,
                     const uint8_t* data,
                     uint16_t size) {
  if (!size)
    return false;
  if (hid_info->type == HID_TYPE_XBOX_360 && size != 20 && data[0] != 0x00)
    return true;
  if (hid_info->type == HID_TYPE_XBOX_ONE && size != 18 && data[0] != 0x20)
    return true;
  return false;
}

void hid_xbox_360_poll(uint8_t hub, struct usb_info* usb_info) {
  if (usb_info->state == CONNECTED) {
    static uint8_t initialize[] = {0x01, 0x03, 0x00};
    initialize[2] = 0x02 + hub;
    usb_host_out(hub, usb_info->ep_out, initialize, sizeof(initialize));
    usb_info->state = INITIALIZED;
  } else if (usb_info->state == INITIALIZED) {
    usb_host_in(hub, usb_info->ep_in, 20);
  }
}

void hid_xbox_one_poll(uint8_t hub, struct usb_info* usb_info) {
  if (usb_info->state == CONNECTED) {
    static uint8_t initialize[] = {0x05, 0x20, 0x00, 0x01, 0x00};
    initialize[2] = usb_info->cmd_count++;
    usb_host_out(hub, usb_info->ep_out, initialize, sizeof(initialize));
    usb_info->state = INITIALIZED;
  } else if (usb_info->state == INITIALIZED) {
    static uint8_t start[] = {0x06, 0x20, 0x00, 0x02, 0x01, 0x00};
    start[2] = usb_info->cmd_count++;
    usb_host_out(hub, usb_info->ep_out, start, sizeof(start));
    usb_info->state = STARTED;
  } else if (usb_info->state == STARTED) {
    usb_host_in(hub, usb_info->ep_in, usb_info->ep_max_packet_size);
  }
}
