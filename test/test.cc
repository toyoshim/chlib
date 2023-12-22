// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include <stdint.h>

extern "C" {
#include "serial.h"
#include "usb/hid/hid.h"
#include "usb/usb.h"
}

#include "gtest/gtest.h"
#include "mock.h"

namespace anonymous {

usb_desc_device usb_device_desc = {
    sizeof(usb_desc_device),
    USB_DESC_DEVICE,
    0x110,
    3,
};
struct DummyConfigurationDescriptor {
  usb_desc_configuration configuration = {
      sizeof(usb_desc_configuration),
      USB_DESC_CONFIGURATION,
      sizeof(DummyConfigurationDescriptor),
  };
  usb_desc_interface interface = {
      sizeof(usb_desc_interface) + 1, USB_DESC_INTERFACE, 1, 0, 1, 3, 0, 0, 1,
  };
  usb_desc_endpoint endpoint = {
      sizeof(usb_desc_endpoint),
      USB_DESC_ENDPOINT,
      129,
      3,
  };
  usb_desc_hid hid = {
      sizeof(usb_desc_hid), USB_DESC_HID, 0x0101, 0x00, 0x01,
      USB_DESC_HID_REPORT,  0x0000,
  };
} usb_conf_desc;

class CompatTest : public ::testing::Test {
 protected:
  void SetVendorAndProduct(uint16_t vid, uint16_t pid) {
    usb_device_desc.idVendor = vid;
    usb_device_desc.idProduct = pid;
  }
  void SetDevice(uint16_t device) { usb_device_desc.bcdDevice = device; }
  void SetReportSize(uint16_t report_size) {
    ASSERT_TRUE(usb_host);
    ASSERT_TRUE(usb_host->check_device_desc);
    ASSERT_TRUE(usb_host->check_configuration_desc);

    usb_host->check_device_desc(
        0, reinterpret_cast<const uint8_t*>(&usb_device_desc));
    usb_conf_desc.hid.wDescriptorLength = report_size;
    usb_host->check_configuration_desc(
        0, reinterpret_cast<const uint8_t*>(&usb_conf_desc));
  }

  void CheckHidReportDescriptor(const uint8_t* desc) {
    ASSERT_TRUE(usb_host->check_hid_report_desc);

    usb_host->check_hid_report_desc(0, desc);
  }

  void CheckHidInfo(hid_info& expected, hid_info& actual) {
    EXPECT_EQ(expected.report_desc_size, actual.report_desc_size);
    EXPECT_EQ(expected.report_size, actual.report_size);

    for (size_t i = 0; i < 6; ++i) {
      EXPECT_EQ(expected.axis[i], actual.axis[i]);
      if (expected.axis[i] == 0xffff)
        continue;
      EXPECT_EQ(expected.axis_size[i], actual.axis_size[i]);
      EXPECT_EQ(expected.axis_sign[i], actual.axis_sign[i]);
      EXPECT_EQ(expected.axis_polarity[i], actual.axis_polarity[i]);
    }

    for (size_t i = 0; i < 4; ++i)
      EXPECT_EQ(expected.dpad[i], actual.dpad[i]);

    for (size_t i = 0; i < 13; ++i)
      EXPECT_EQ(expected.button[i], actual.button[i]);
    EXPECT_EQ(expected.hat, actual.hat);

    EXPECT_EQ(expected.report_id, actual.report_id);
    EXPECT_EQ(expected.type, actual.type);
    EXPECT_EQ(expected.state, actual.state);
  }

 private:
  void SetUp() override {
    serial_init();
    memset(&hid, 0, sizeof(hid));
    hid_init(&hid);
    SetVendorAndProduct(0, 0);
  }

  struct hid hid;
};

// Compatibility tests for PS4 controllers with precised descriptors
using PS4CompatTest = CompatTest;

TEST_F(PS4CompatTest, HoripadFpsPlusForPlayStation4_ModePS4) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31,
      0x09, 0x32, 0x09, 0x35, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95,
      0x04, 0x81, 0x02, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46,
      0x3b, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00,
      0x05, 0x09, 0x19, 0x01, 0x29, 0x0e, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
      0x95, 0x0e, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09, 0x20, 0x75, 0x06, 0x95,
      0x01, 0x81, 0x02, 0x05, 0x01, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26,
      0xff, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09,
      0x21, 0x95, 0x36, 0x81, 0x02, 0x85, 0x05, 0x09, 0x22, 0x95, 0x1f, 0x91,
      0x02, 0x85, 0x03, 0x0a, 0x21, 0x27, 0x95, 0x2f, 0xb1, 0x02, 0xc0, 0x06,
      0xf0, 0xff, 0x09, 0x40, 0xa1, 0x01, 0x85, 0xf0, 0x09, 0x47, 0x95, 0x3f,
      0xb1, 0x02, 0x85, 0xf1, 0x09, 0x48, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf2,
      0x09, 0x49, 0x95, 0x0f, 0xb1, 0x02, 0x85, 0xf3, 0x0a, 0x01, 0x47, 0x95,
      0x07, 0xb1, 0x02, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      504,
      {0, 8, 16, 24, 56, 64},
      32,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      1,
      HID_TYPE_PS4,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(PS4CompatTest, HoripadFpsPlusForPlayStation4_ModePS3) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x15, 0x00, 0x25, 0x01, 0x35, 0x00,
      0x45, 0x01, 0x75, 0x01, 0x95, 0x0d, 0x05, 0x09, 0x19, 0x01, 0x29, 0x0d,
      0x81, 0x02, 0x95, 0x03, 0x81, 0x01, 0x05, 0x01, 0x25, 0x07, 0x46, 0x3b,
      0x01, 0x75, 0x04, 0x95, 0x01, 0x65, 0x14, 0x09, 0x39, 0x81, 0x42, 0x65,
      0x00, 0x95, 0x01, 0x81, 0x01, 0x26, 0xff, 0x00, 0x46, 0xff, 0x00, 0x09,
      0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35, 0x75, 0x08, 0x95, 0x04, 0x81,
      0x02, 0x06, 0x00, 0xff, 0x09, 0x20, 0x09, 0x21, 0x09, 0x22, 0x09, 0x23,
      0x09, 0x24, 0x09, 0x25, 0x09, 0x26, 0x09, 0x27, 0x09, 0x28, 0x09, 0x29,
      0x09, 0x2a, 0x09, 0x2b, 0x95, 0x0c, 0x81, 0x02, 0x0a, 0x21, 0x26, 0x95,
      0x08, 0xb1, 0x02, 0x0a, 0x21, 0x26, 0x91, 0x02, 0x26, 0xff, 0x03, 0x46,
      0xff, 0x03, 0x09, 0x2c, 0x09, 0x2d, 0x09, 0x2e, 0x09, 0x2f, 0x75, 0x10,
      0x95, 0x04, 0x81, 0x02, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      216,
      {24, 32, 40, 48, 56, 64},
      16,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(PS4CompatTest, WirelessController_DualShock4_CUH_ZCT2J) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31,
      0x09, 0x32, 0x09, 0x35, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95,
      0x04, 0x81, 0x02, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46,
      0x3b, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00,
      0x05, 0x09, 0x19, 0x01, 0x29, 0x0e, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
      0x95, 0x0e, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09, 0x20, 0x75, 0x06, 0x95,
      0x01, 0x15, 0x00, 0x25, 0x7f, 0x81, 0x02, 0x05, 0x01, 0x09, 0x33, 0x09,
      0x34, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02,
      0x06, 0x00, 0xff, 0x09, 0x21, 0x95, 0x36, 0x81, 0x02, 0x85, 0x05, 0x09,
      0x22, 0x95, 0x1f, 0x91, 0x02, 0x85, 0x04, 0x09, 0x23, 0x95, 0x24, 0xb1,
      0x02, 0x85, 0x02, 0x09, 0x24, 0x95, 0x24, 0xb1, 0x02, 0x85, 0x08, 0x09,
      0x25, 0x95, 0x03, 0xb1, 0x02, 0x85, 0x10, 0x09, 0x26, 0x95, 0x04, 0xb1,
      0x02, 0x85, 0x11, 0x09, 0x27, 0x95, 0x02, 0xb1, 0x02, 0x85, 0x12, 0x06,
      0x02, 0xff, 0x09, 0x21, 0x95, 0x0f, 0xb1, 0x02, 0x85, 0x13, 0x09, 0x22,
      0x95, 0x16, 0xb1, 0x02, 0x85, 0x14, 0x06, 0x05, 0xff, 0x09, 0x20, 0x95,
      0x10, 0xb1, 0x02, 0x85, 0x15, 0x09, 0x21, 0x95, 0x2c, 0xb1, 0x02, 0x06,
      0x80, 0xff, 0x85, 0x80, 0x09, 0x20, 0x95, 0x06, 0xb1, 0x02, 0x85, 0x81,
      0x09, 0x21, 0x95, 0x06, 0xb1, 0x02, 0x85, 0x82, 0x09, 0x22, 0x95, 0x05,
      0xb1, 0x02, 0x85, 0x83, 0x09, 0x23, 0x95, 0x01, 0xb1, 0x02, 0x85, 0x84,
      0x09, 0x24, 0x95, 0x04, 0xb1, 0x02, 0x85, 0x85, 0x09, 0x25, 0x95, 0x06,
      0xb1, 0x02, 0x85, 0x86, 0x09, 0x26, 0x95, 0x06, 0xb1, 0x02, 0x85, 0x87,
      0x09, 0x27, 0x95, 0x23, 0xb1, 0x02, 0x85, 0x88, 0x09, 0x28, 0x95, 0x3f,
      0xb1, 0x02, 0x85, 0x89, 0x09, 0x29, 0x95, 0x02, 0xb1, 0x02, 0x85, 0x90,
      0x09, 0x30, 0x95, 0x05, 0xb1, 0x02, 0x85, 0x91, 0x09, 0x31, 0x95, 0x03,
      0xb1, 0x02, 0x85, 0x92, 0x09, 0x32, 0x95, 0x03, 0xb1, 0x02, 0x85, 0x93,
      0x09, 0x33, 0x95, 0x0c, 0xb1, 0x02, 0x85, 0x94, 0x09, 0x34, 0x95, 0x3f,
      0xb1, 0x02, 0x85, 0xa0, 0x09, 0x40, 0x95, 0x06, 0xb1, 0x02, 0x85, 0xa1,
      0x09, 0x41, 0x95, 0x01, 0xb1, 0x02, 0x85, 0xa2, 0x09, 0x42, 0x95, 0x01,
      0xb1, 0x02, 0x85, 0xa3, 0x09, 0x43, 0x95, 0x30, 0xb1, 0x02, 0x85, 0xa4,
      0x09, 0x44, 0x95, 0x0d, 0xb1, 0x02, 0x85, 0xf0, 0x09, 0x47, 0x95, 0x3f,
      0xb1, 0x02, 0x85, 0xf1, 0x09, 0x48, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf2,
      0x09, 0x49, 0x95, 0x0f, 0xb1, 0x02, 0x85, 0xa7, 0x09, 0x4a, 0x95, 0x01,
      0xb1, 0x02, 0x85, 0xa8, 0x09, 0x4b, 0x95, 0x01, 0xb1, 0x02, 0x85, 0xa9,
      0x09, 0x4c, 0x95, 0x08, 0xb1, 0x02, 0x85, 0xaa, 0x09, 0x4e, 0x95, 0x01,
      0xb1, 0x02, 0x85, 0xab, 0x09, 0x4f, 0x95, 0x39, 0xb1, 0x02, 0x85, 0xac,
      0x09, 0x50, 0x95, 0x39, 0xb1, 0x02, 0x85, 0xad, 0x09, 0x51, 0x95, 0x0b,
      0xb1, 0x02, 0x85, 0xae, 0x09, 0x52, 0x95, 0x01, 0xb1, 0x02, 0x85, 0xaf,
      0x09, 0x53, 0x95, 0x02, 0xb1, 0x02, 0x85, 0xb0, 0x09, 0x54, 0x95, 0x3f,
      0xb1, 0x02, 0x85, 0xe0, 0x09, 0x57, 0x95, 0x02, 0xb1, 0x02, 0x85, 0xb3,
      0x09, 0x55, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xb4, 0x09, 0x55, 0x95, 0x3f,
      0xb1, 0x02, 0x85, 0xb5, 0x09, 0x56, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xd0,
      0x09, 0x58, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xd4, 0x09, 0x59, 0x95, 0x3f,
      0xb1, 0x02, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      504,
      {0, 8, 16, 24, 56, 64},
      32,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      1,
      HID_TYPE_PS4,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(PS4CompatTest, UniversalFightingBoard) {
  // This seems emuloating Horipad FPS+.
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31,
      0x09, 0x32, 0x09, 0x35, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95,
      0x04, 0x81, 0x02, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46,
      0x3b, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00,
      0x05, 0x09, 0x19, 0x01, 0x29, 0x0e, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
      0x95, 0x0e, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09, 0x20, 0x75, 0x06, 0x95,
      0x01, 0x81, 0x02, 0x05, 0x01, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26,
      0xff, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09,
      0x21, 0x95, 0x36, 0x81, 0x02, 0x85, 0x05, 0x09, 0x22, 0x95, 0x1f, 0x91,
      0x02, 0x85, 0x03, 0x0a, 0x21, 0x27, 0x95, 0x2f, 0xb1, 0x02, 0xc0, 0x06,
      0xf0, 0xff, 0x09, 0x40, 0xa1, 0x01, 0x85, 0xf0, 0x09, 0x47, 0x95, 0x3f,
      0xb1, 0x02, 0x85, 0xf1, 0x09, 0x48, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf2,
      0x09, 0x49, 0x95, 0x0f, 0xb1, 0x02, 0x85, 0xf3, 0x0a, 0x01, 0x47, 0x95,
      0x07, 0xb1, 0x02, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      504,
      {0, 8, 16, 24, 56, 64},
      32,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      1,
      HID_TYPE_PS4,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(PS4CompatTest, PS4PlusFightingBoard) {
  // This seems emuloating Horipad FPS+.
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31,
      0x09, 0x32, 0x09, 0x35, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95,
      0x04, 0x81, 0x02, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46,
      0x3b, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00,
      0x05, 0x09, 0x19, 0x01, 0x29, 0x0e, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
      0x95, 0x0e, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09, 0x20, 0x75, 0x06, 0x95,
      0x01, 0x81, 0x02, 0x05, 0x01, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26,
      0xff, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09,
      0x21, 0x95, 0x36, 0x81, 0x02, 0x85, 0x05, 0x09, 0x22, 0x95, 0x1f, 0x91,
      0x02, 0x85, 0x03, 0x0a, 0x21, 0x27, 0x95, 0x2f, 0xb1, 0x02, 0x06, 0x80,
      0xff, 0x85, 0xe0, 0x09, 0x57, 0x95, 0x02, 0xb1, 0x02, 0xc0, 0x06, 0xf0,
      0xff, 0x09, 0x40, 0xa1, 0x01, 0x85, 0xf0, 0x09, 0x47, 0x95, 0x3f, 0xb1,
      0x02, 0x85, 0xf1, 0x09, 0x48, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf2, 0x09,
      0x49, 0x95, 0x0f, 0xb1, 0x02, 0x85, 0xf3, 0x0a, 0x01, 0x47, 0x95, 0x07,
      0xb1, 0x02, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      504,
      {0, 8, 16, 24, 56, 64},
      32,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      1,
      HID_TYPE_PS4,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(PS4CompatTest, G29DrivingForceRacingWheelPS3) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0xa1, 0x02, 0x15, 0x00, 0x25, 0x01,
      0x35, 0x00, 0x45, 0x01, 0x75, 0x01, 0x95, 0x0d, 0x05, 0x09, 0x19, 0x01,
      0x29, 0x0d, 0x81, 0x02, 0x95, 0x03, 0x81, 0x01, 0x05, 0x01, 0x25, 0x07,
      0x46, 0x3b, 0x01, 0x75, 0x04, 0x95, 0x01, 0x65, 0x14, 0x09, 0x39, 0x81,
      0x42, 0x65, 0x00, 0x95, 0x01, 0x81, 0x01, 0x26, 0xff, 0x00, 0x46, 0xff,
      0x00, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35, 0x75, 0x08, 0x95,
      0x04, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09, 0x20, 0x09, 0x21, 0x09, 0x22,
      0x09, 0x23, 0x09, 0x24, 0x09, 0x25, 0x09, 0x26, 0x09, 0x27, 0x09, 0x28,
      0x09, 0x29, 0x09, 0x2a, 0x09, 0x2b, 0x95, 0x0c, 0x81, 0x02, 0x0a, 0x21,
      0x26, 0x95, 0x08, 0xb1, 0x02, 0x0a, 0x21, 0x26, 0x91, 0x02, 0x26, 0xff,
      0x03, 0x46, 0xff, 0x03, 0x09, 0x2c, 0x09, 0x2d, 0x09, 0x2e, 0x09, 0x2f,
      0x75, 0x10, 0x95, 0x04, 0x81, 0x02, 0xc0, 0xa1, 0x02, 0x26, 0xff, 0x00,
      0x46, 0xff, 0x00, 0x95, 0x07, 0x75, 0x08, 0x09, 0x03, 0x91, 0x02, 0xc0,
      0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      216,
      {24, 136, 144, 48, 56, 64},
      16,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_PS4,
      HID_STATE_READY,
  };

  SetVendorAndProduct(0x046d, 0xc294);
  SetDevice(0x1350);
  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(PS4CompatTest, G29DrivingForceRacingWheelPS4) {
  // This seems the same with Horipad FPS+.
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x04, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x30, 0x09, 0x31,
      0x09, 0x32, 0x09, 0x35, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95,
      0x04, 0x81, 0x02, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46,
      0x3b, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x65, 0x00,
      0x05, 0x09, 0x19, 0x01, 0x29, 0x0e, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
      0x95, 0x0e, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09, 0x20, 0x75, 0x06, 0x95,
      0x01, 0x81, 0x02, 0x05, 0x01, 0x09, 0x33, 0x09, 0x34, 0x15, 0x00, 0x26,
      0xff, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0x06, 0x00, 0xff, 0x09,
      0x21, 0x95, 0x36, 0x81, 0x02, 0x85, 0x05, 0x09, 0x22, 0x95, 0x1f, 0x91,
      0x02, 0x85, 0x03, 0x0a, 0x21, 0x27, 0x95, 0x2f, 0xb1, 0x02, 0xc0, 0x06,
      0xf0, 0xff, 0x09, 0x40, 0xa1, 0x01, 0x85, 0xf0, 0x09, 0x47, 0x95, 0x3f,
      0xb1, 0x02, 0x85, 0xf1, 0x09, 0x48, 0x95, 0x3f, 0xb1, 0x02, 0x85, 0xf2,
      0x09, 0x49, 0x95, 0x0f, 0xb1, 0x02, 0x85, 0xf3, 0x0a, 0x01, 0x47, 0x95,
      0x07, 0xb1, 0x02, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      504,
      {336, 352, 368, 384, 56, 64},
      32,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48},
      {16, 16, 16, 16, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, true, true, true, false, false},
      1,
      HID_TYPE_PS4,
      HID_STATE_READY,
  };

  SetVendorAndProduct(0x046d, 0xc260);
  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// Compatibility tests for PS4 controllers with pseudo descriptors
using PS4PseudoCompatTest = CompatTest;

TEST_F(PS4PseudoCompatTest, WirelessController_DualShock4_CUH_ZCT1J) {
  const uint8_t pseudo_hid_report_desc[] = {
      0x85, 0x01, 0x95, 0x04, 0x75, 0x08, 0x05, 0x01, 0x09, 0x30, 0x05, 0x01,
      0x09, 0x31, 0x05, 0x01, 0x09, 0x32, 0x05, 0x01, 0x09, 0x35, 0x81, 0x02,
      0x95, 0x01, 0x75, 0x04, 0x05, 0x01, 0x09, 0x39, 0x81, 0x42, 0x95, 0x0e,
      0x75, 0x01, 0x2a, 0x0e, 0x00, 0x1a, 0x01, 0x00, 0x81, 0x02, 0x95, 0x01,
      0x75, 0x06, 0x06, 0x00, 0xff, 0x09, 0x20, 0x81, 0x02, 0x95, 0x02, 0x75,
      0x08, 0x05, 0x01, 0x09, 0x33, 0x05, 0x01, 0x09, 0x34, 0x81, 0x02, 0x95,
      0x36, 0x75, 0x08, 0x06, 0x00, 0xff, 0x09, 0x21, 0x81, 0x02,
  };
  hid_info expected = {
      sizeof(pseudo_hid_report_desc),
      504,
      {0, 8, 16, 24, 56, 64},
      32,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      1,
      HID_TYPE_PS4,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(pseudo_hid_report_desc));
  CheckHidReportDescriptor(pseudo_hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(PS4PseudoCompatTest, WirelessController_DualShock4_CUH_ZCT2J) {
  const uint8_t pseudo_hid_report_desc[] = {
      0x85, 0x01, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x30, 0x81, 0x02,
      0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x31, 0x81, 0x02, 0x95, 0x01,
      0x75, 0x08, 0x05, 0x01, 0x09, 0x32, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08,
      0x05, 0x01, 0x09, 0x35, 0x81, 0x02, 0x95, 0x01, 0x75, 0x04, 0x05, 0x01,
      0x09, 0x39, 0x81, 0x42, 0x95, 0x0e, 0x75, 0x01, 0x2a, 0x0e, 0x00, 0x1a,
      0x01, 0x00, 0x81, 0x02, 0x95, 0x01, 0x75, 0x06, 0x06, 0x00, 0xff, 0x09,
      0x20, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x33, 0x81,
      0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x34, 0x81, 0x02, 0x95,
      0x36, 0x75, 0x08, 0x06, 0x00, 0xff, 0x09, 0x21, 0x81, 0x02,
  };
  hid_info expected = {
      sizeof(pseudo_hid_report_desc),
      504,
      {0, 8, 16, 24, 56, 64},
      32,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      1,
      HID_TYPE_PS4,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(pseudo_hid_report_desc));
  CheckHidReportDescriptor(pseudo_hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(PS4PseudoCompatTest, VictrixProFSwithTouchPadForPS4) {
  const uint8_t pseudo_hid_report_desc[] = {
      0x85, 0x01, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x30, 0x81, 0x02,
      0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x31, 0x81, 0x02, 0x95, 0x01,
      0x75, 0x08, 0x05, 0x01, 0x09, 0x32, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08,
      0x05, 0x01, 0x09, 0x35, 0x81, 0x02, 0x95, 0x01, 0x75, 0x04, 0x05, 0x01,
      0x09, 0x39, 0x81, 0x42, 0x95, 0x0e, 0x75, 0x01, 0x2a, 0x0e, 0x00, 0x1a,
      0x01, 0x00, 0x81, 0x02, 0x95, 0x01, 0x75, 0x06, 0x06, 0x00, 0xff, 0x09,
      0x20, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x33, 0x81,
      0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x34, 0x81, 0x02, 0x95,
      0x36, 0x75, 0x08, 0x06, 0x00, 0xff, 0x09, 0x21, 0x81, 0x02,
  };
  hid_info expected = {
      sizeof(pseudo_hid_report_desc),
      504,
      {0, 8, 16, 24, 56, 64},
      32,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      1,
      HID_TYPE_PS4,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(pseudo_hid_report_desc));
  CheckHidReportDescriptor(pseudo_hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

using PS3CompatTest = CompatTest;
TEST_F(PS3CompatTest, DualShock3) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x04, 0xa1, 0x01, 0xa1, 0x02, 0x85, 0x01, 0x75, 0x08,
      0x95, 0x01, 0x15, 0x00, 0x26, 0xff, 0x00, 0x81, 0x03, 0x75, 0x01, 0x95,
      0x13, 0x15, 0x00, 0x25, 0x01, 0x35, 0x00, 0x45, 0x01, 0x05, 0x09, 0x19,
      0x01, 0x29, 0x13, 0x81, 0x02, 0x75, 0x01, 0x95, 0x0d, 0x06, 0x00, 0xff,
      0x81, 0x03, 0x15, 0x00, 0x26, 0xff, 0x00, 0x05, 0x01, 0x09, 0x01, 0xa1,
      0x00, 0x75, 0x08, 0x95, 0x04, 0x35, 0x00, 0x46, 0xff, 0x00, 0x09, 0x30,
      0x09, 0x31, 0x09, 0x32, 0x09, 0x35, 0x81, 0x02, 0xc0, 0x05, 0x01, 0x75,
      0x08, 0x95, 0x27, 0x09, 0x01, 0x81, 0x02, 0x75, 0x08, 0x95, 0x30, 0x09,
      0x01, 0x91, 0x02, 0x75, 0x08, 0x95, 0x30, 0x09, 0x01, 0xb1, 0x02, 0xc0,
      0xa1, 0x02, 0x85, 0x02, 0x75, 0x08, 0x95, 0x30, 0x09, 0x01, 0xb1, 0x02,
      0xc0, 0xa1, 0x02, 0x85, 0xee, 0x75, 0x08, 0x95, 0x30, 0x09, 0x01, 0xb1,
      0x02, 0xc0, 0xa1, 0x02, 0x85, 0xef, 0x75, 0x08, 0x95, 0x30, 0x09, 0x01,
      0xb1, 0x02, 0xc0, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      384,
      {40, 48, 56, 64, 136, 144},
      0xffff,
      {12, 14, 15, 13},
      {23, 22, 21, 20, 18, 19, 16, 17, 8, 11, 9, 10, 24},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      1,
      HID_TYPE_PS3,
      HID_STATE_READY,
  };

  SetVendorAndProduct(0x054c, 0x0268);
  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// Compatibility tests for PS3 controllers with pseudo descriptors
using PS3PseudoCompatTest = CompatTest;

// VID: 0x16c0
// PID: 0x05e1
TEST_F(PS3PseudoCompatTest, XinMoController) {
  const uint8_t pseudo_hid_report_desc[] = {
      0x85, 0x00, 0x95, 0x0d, 0x75, 0x01, 0x2a, 0x0d, 0x00, 0x1a, 0x01,
      0x00, 0x81, 0x02, 0x95, 0x01, 0x75, 0x03, 0x81, 0x01, 0x95, 0x01,
      0x75, 0x04, 0x05, 0x01, 0x09, 0x39, 0x81, 0x42, 0x95, 0x01, 0x75,
      0x04, 0x81, 0x01, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x30,
      0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x31, 0x81,
      0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x32, 0x81, 0x02,
      0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x35, 0x81, 0x02,
  };
  hid_info expected = {
      sizeof(pseudo_hid_report_desc),
      56,
      {24, 32, 40, 48, 0xffff, 0xffff},
      16,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(pseudo_hid_report_desc));
  CheckHidReportDescriptor(pseudo_hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// VID: 0x0f0d (HORI CO,.LTD.)
// PID: 0x003d
TEST_F(PS3PseudoCompatTest, RAP_N3) {
  const uint8_t pseudo_hid_report_desc[] = {
      0x85, 0x00, 0x95, 0x0d, 0x75, 0x01, 0x2a, 0x0d, 0x00, 0x1a, 0x01, 0x00,
      0x81, 0x02, 0x95, 0x01, 0x75, 0x03, 0x81, 0x01, 0x95, 0x01, 0x75, 0x04,
      0x05, 0x01, 0x09, 0x39, 0x81, 0x42, 0x95, 0x01, 0x75, 0x04, 0x81, 0x01,
      0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x30, 0x81, 0x02, 0x95, 0x01,
      0x75, 0x08, 0x05, 0x01, 0x09, 0x31, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08,
      0x05, 0x01, 0x09, 0x32, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01,
      0x09, 0x35, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x06, 0x00, 0xff, 0x09,
      0x20, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x06, 0x00, 0xff, 0x09, 0x21,
      0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x06, 0x00, 0xff, 0x09, 0x22, 0x81,
      0x02, 0x95, 0x01, 0x75, 0x08, 0x06, 0x00, 0xff, 0x09, 0x23, 0x81, 0x02,
      0x95, 0x01, 0x75, 0x08, 0x06, 0x00, 0xff, 0x09, 0x24, 0x81, 0x02, 0x95,
      0x01, 0x75, 0x08, 0x06, 0x00, 0xff, 0x09, 0x25, 0x81, 0x02, 0x95, 0x01,
      0x75, 0x08, 0x06, 0x00, 0xff, 0x09, 0x26, 0x81, 0x02, 0x95, 0x01, 0x75,
      0x08, 0x06, 0x00, 0xff, 0x09, 0x27, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08,
      0x06, 0x00, 0xff, 0x09, 0x28, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x06,
      0x00, 0xff, 0x09, 0x29, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x06, 0x00,
      0xff, 0x09, 0x2a, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x06, 0x00, 0xff,
      0x09, 0x2b, 0x81, 0x02, 0x95, 0x01, 0x75, 0x10, 0x06, 0x00, 0xff, 0x09,
      0x2c, 0x81, 0x02, 0x95, 0x01, 0x75, 0x10, 0x06, 0x00, 0xff, 0x09, 0x2d,
      0x81, 0x02, 0x95, 0x01, 0x75, 0x10, 0x06, 0x00, 0xff, 0x09, 0x2e, 0x81,
      0x02, 0x95, 0x01, 0x75, 0x10, 0x06, 0x00, 0xff, 0x09, 0x2f, 0x81, 0x02,
  };
  hid_info expected = {
      sizeof(pseudo_hid_report_desc),
      216,
      {24, 32, 40, 48, 56, 64},
      16,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(pseudo_hid_report_desc));
  CheckHidReportDescriptor(pseudo_hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// Compatibility tests for Switch controllers with precised descriptors
using SwitchCompatTest = CompatTest;

TEST_F(SwitchCompatTest, HoripadMiniForNintendoSwitch) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x15, 0x00, 0x25, 0x01, 0x35, 0x00,
      0x45, 0x01, 0x75, 0x01, 0x95, 0x0c, 0x05, 0x09, 0x19, 0x01, 0x29, 0x0c,
      0x81, 0x02, 0x95, 0x04, 0x81, 0x01, 0x05, 0x01, 0x25, 0x07, 0x46, 0x3b,
      0x01, 0x75, 0x04, 0x95, 0x01, 0x65, 0x14, 0x09, 0x39, 0x81, 0x42, 0x65,
      0x00, 0x95, 0x01, 0x81, 0x01, 0x26, 0xff, 0x00, 0x46, 0xff, 0x00, 0x09,
      0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35, 0x75, 0x08, 0x95, 0x04, 0x81,
      0x02, 0x06, 0x00, 0xff, 0x09, 0x20, 0x09, 0x21, 0x09, 0x22, 0x09, 0x23,
      0x09, 0x24, 0x09, 0x25, 0x09, 0x26, 0x09, 0x27, 0x09, 0x28, 0x09, 0x29,
      0x09, 0x2a, 0x09, 0x2b, 0x95, 0x0c, 0x81, 0x02, 0x26, 0xff, 0x03, 0x46,
      0xff, 0x03, 0x09, 0x2c, 0x09, 0x2d, 0x09, 0x2e, 0x09, 0x2f, 0x75, 0x10,
      0x95, 0x04, 0x81, 0x02, 0x05, 0x08, 0x09, 0x43, 0x15, 0x00, 0x26, 0xff,
      0x00, 0x35, 0x00, 0x46, 0xff, 0x00, 0x75, 0x08, 0x95, 0x01, 0x91, 0x82,
      0x09, 0x44, 0x91, 0x82, 0x09, 0x45, 0x91, 0x82, 0x09, 0x46, 0x91, 0x82,
      0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      216,
      {24, 32, 40, 48, 56, 64},
      16,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0xffff},
      {8, 8, 8, 8, 8, 8},
      {0, 0, 0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(SwitchCompatTest, RetroArcadeStick) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x15, 0x00, 0x25, 0x01, 0x35, 0x00,
      0x45, 0x01, 0x75, 0x01, 0x95, 0x0e, 0x05, 0x09, 0x19, 0x01, 0x29, 0x0e,
      0x81, 0x02, 0x95, 0x02, 0x81, 0x01, 0x05, 0x01, 0x25, 0x07, 0x46, 0x01,
      0x3b, 0x75, 0x04, 0x95, 0x01, 0x65, 0x14, 0x09, 0x39, 0x81, 0x42, 0x65,
      0x00, 0x95, 0x01, 0x81, 0x01, 0x26, 0x00, 0xff, 0x46, 0x00, 0xff, 0x09,
      0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35, 0x75, 0x08, 0x95, 0x04, 0x81,
      0x02, 0x75, 0x08, 0x95, 0x01, 0x81, 0x01, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      64,
      {24, 32, 40, 48, 0xffff, 0xffff},
      16,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {8, 8, 8, 8},
      {0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// VID: 0x0f0d (HORI CO,.LTD.)
// PID: 0x00aa
// Note: These IDs are the same with Real Arcade Pro V Hayabusa in Switch Mode
TEST_F(SwitchCompatTest, ZeroPiFightingBoard) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x15, 0x00, 0x25, 0x01, 0x35, 0x00,
      0x45, 0x01, 0x75, 0x01, 0x95, 0x0e, 0x05, 0x09, 0x19, 0x01, 0x29, 0x0e,
      0x81, 0x02, 0x95, 0x02, 0x81, 0x01, 0x05, 0x01, 0x25, 0x07, 0x46, 0x3b,
      0x01, 0x75, 0x04, 0x95, 0x01, 0x65, 0x14, 0x09, 0x39, 0x81, 0x42, 0x65,
      0x00, 0x95, 0x01, 0x81, 0x01, 0x26, 0xff, 0x00, 0x46, 0xff, 0x00, 0x09,
      0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35, 0x75, 0x08, 0x95, 0x04, 0x81,
      0x02, 0x75, 0x08, 0x95, 0x01, 0x81, 0x01, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      64,
      {24, 32, 40, 48, 0xffff, 0xffff},
      16,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {8, 8, 8, 8},
      {0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// Compatibility tests for switch controllers with pseudo descriptors
using SwitchPseudoCompatTest = CompatTest;

// VID: 0x0e6f
// PID: 0x0180
TEST_F(SwitchPseudoCompatTest, RetroArcadeStick) {
  const uint8_t pseudo_hid_report_desc[] = {
      0x85, 0x00, 0x95, 0x0e, 0x75, 0x01, 0x2a, 0x0e, 0x00, 0x1a, 0x01, 0x00,
      0x81, 0x02, 0x95, 0x01, 0x75, 0x02, 0x81, 0x01, 0x95, 0x01, 0x75, 0x04,
      0x05, 0x01, 0x09, 0x39, 0x81, 0x42, 0x95, 0x01, 0x75, 0x04, 0x81, 0x01,
      0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x30, 0x81, 0x02, 0x95, 0x01,
      0x75, 0x08, 0x05, 0x01, 0x09, 0x31, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08,
      0x05, 0x01, 0x09, 0x32, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01,
      0x09, 0x35, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
  };
  hid_info expected = {
      sizeof(pseudo_hid_report_desc),
      64,
      {24, 32, 40, 48, 0xffff, 0xffff},
      16,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {8, 8, 8, 8},
      {0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(pseudo_hid_report_desc));
  CheckHidReportDescriptor(pseudo_hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// Compatibility tests for other controllers with precised descriptors
using GenericCompatTest = CompatTest;

TEST_F(GenericCompatTest, 6BController_MegaDriveMini) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x04, 0xa1, 0x01, 0xa1, 0x02, 0x75, 0x08, 0x95, 0x05,
      0x15, 0x00, 0x26, 0xff, 0x00, 0x35, 0x00, 0x46, 0xff, 0x00, 0x09, 0x30,
      0x09, 0x30, 0x09, 0x30, 0x09, 0x30, 0x09, 0x31, 0x81, 0x02, 0x75, 0x04,
      0x95, 0x01, 0x25, 0x07, 0x46, 0x3b, 0x01, 0x65, 0x14, 0x09, 0x00, 0x81,
      0x42, 0x65, 0x00, 0x75, 0x01, 0x95, 0x0a, 0x25, 0x01, 0x45, 0x01, 0x05,
      0x09, 0x19, 0x01, 0x29, 0x0a, 0x81, 0x02, 0x06, 0x00, 0xff, 0x75, 0x01,
      0x95, 0x0a, 0x25, 0x01, 0x45, 0x01, 0x09, 0x01, 0x81, 0x02, 0xc0, 0xa1,
      0x02, 0x75, 0x08, 0x95, 0x04, 0x46, 0xff, 0x00, 0x26, 0xff, 0x00, 0x09,
      0x02, 0x91, 0x02, 0xc0, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      64,
      {24, 32, 40, 0xffff, 0xffff, 0xffff},
      0xffff,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56},
      {8, 8, 4},
      {0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

TEST_F(GenericCompatTest, XE1AJ_USB_MegaDriveMini2) {
  const uint8_t hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x15, 0x00, 0x25, 0x01, 0x35,
      0x00, 0x45, 0x01, 0x75, 0x01, 0x95, 0x0d, 0x05, 0x09, 0x19, 0x01,
      0x29, 0x0d, 0x81, 0x02, 0x95, 0x03, 0x81, 0x01, 0x05, 0x01, 0x25,
      0x07, 0x46, 0x3b, 0x01, 0x75, 0x04, 0x95, 0x01, 0x65, 0x14, 0x09,
      0x39, 0x81, 0x42, 0x65, 0x00, 0x95, 0x01, 0x81, 0x01, 0x26, 0xff,
      0x00, 0x46, 0xff, 0x00, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09,
      0x35, 0x75, 0x08, 0x95, 0x04, 0x81, 0x02, 0xc0,
  };
  hid_info expected = {
      sizeof(hid_report_desc),
      56,
      {24, 32, 40, 48, 0xffff, 0xffff},
      16,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {8, 8, 8, 8},
      {0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(hid_report_desc));
  CheckHidReportDescriptor(hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// VID: 0x33dd
// PID: 0x0013
TEST_F(GenericCompatTest, X68000Z_JOYCARD_BLACK) {
  const uint8_t pseudo_hid_report_desc[] = {
      0x05, 0x01, 0x09, 0x04, 0xa1, 0x01, 0x15, 0x00, 0x25, 0x01,
      0x35, 0x00, 0x45, 0x01, 0x75, 0x01, 0x95, 0x0c, 0x05, 0x09,
      0x19, 0x01, 0x29, 0x0c, 0x81, 0x02, 0x95, 0x04, 0x81, 0x01,
      0x05, 0x01, 0x26, 0xff, 0x00, 0x46, 0xff, 0x00, 0x09, 0x30,
      0x09, 0x31, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02, 0xc0};
  hid_info expected = {
      sizeof(pseudo_hid_report_desc),
      32,
      {16, 24, 0xffff, 0xffff, 0xffff, 0xffff},
      0xffff,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0xffff},
      {8, 8, 8, 8},
      {0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(pseudo_hid_report_desc));
  CheckHidReportDescriptor(pseudo_hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// Compatibility tests for other controllers with pseudo descriptors
using GenericPseudoCompatTest = CompatTest;

TEST_F(GenericPseudoCompatTest, 6BController_MegaDriveMini) {
  const uint8_t pseudo_hid_report_desc[] = {
      0x85, 0x00, 0x95, 0x05, 0x75, 0x08, 0x05, 0x01, 0x09, 0x30, 0x05,
      0x01, 0x09, 0x30, 0x05, 0x01, 0x09, 0x30, 0x05, 0x01, 0x09, 0x30,
      0x05, 0x01, 0x09, 0x31, 0x81, 0x02, 0x95, 0x01, 0x75, 0x04, 0x05,
      0x01, 0x09, 0x00, 0x81, 0x42, 0x95, 0x0a, 0x75, 0x01, 0x2a, 0x0a,
      0x00, 0x1a, 0x01, 0x00, 0x81, 0x02, 0x95, 0x0a, 0x75, 0x01, 0x06,
      0x00, 0xff, 0x09, 0x01, 0x81, 0x02,
  };
  hid_info expected = {
      sizeof(pseudo_hid_report_desc),
      64,
      {24, 32, 40, 0xffff, 0xffff, 0xffff},
      0xffff,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56},
      {8, 8, 4},
      {0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      0,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(pseudo_hid_report_desc));
  CheckHidReportDescriptor(pseudo_hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

// VID: 0x046d
// PID: 0xc262
TEST_F(GenericPseudoCompatTest, G920DrivingForceRacingWheelForXboxOne) {
  const uint8_t pseudo_hid_report_desc[] = {
      0x85, 0x11, 0x95, 0x13, 0x75, 0x08, 0x06, 0x43, 0xff, 0x09, 0x02, 0x81,
      0x00,  // Collection 0
      0x85, 0x12, 0x95, 0x3f, 0x75, 0x08, 0x06, 0x43, 0xff, 0x09, 0x04, 0x81,
      0x00,  // Collection 1
      0x85, 0x01, 0x95, 0x01, 0x75, 0x04, 0x05, 0x01, 0x09, 0x39, 0x81, 0x42,
      0x95, 0x12, 0x75, 0x01, 0x2a, 0x12, 0x00, 0x1a, 0x01, 0x00, 0x81, 0x02,
      0x95, 0x01, 0x75, 0x02, 0x81, 0x01, 0x95, 0x01, 0x75, 0x10, 0x05, 0x01,
      0x09, 0x30, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x31,
      0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x32, 0x81, 0x02,
      0x95, 0x01, 0x75, 0x08, 0x05, 0x01, 0x09, 0x35, 0x81, 0x02, 0x95, 0x03,
      0x75, 0x01, 0x2a, 0x03, 0x00, 0x1a, 0x01, 0x00, 0x81, 0x02, 0x95, 0x01,
      0x75, 0x05, 0x81, 0x01,  // Collection 2
  };
  hid_info expected = {
      sizeof(pseudo_hid_report_desc),
      72,
      {24, 40, 48, 56, 0xffff, 0xffff},
      0,
      {0xffff, 0xffff, 0xffff, 0xffff},
      {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
      {16, 8, 8, 8},
      {0, 0, 0, 0},
      {false, false, false, false, false, false},
      {false, false, false, false, false, false},
      1,
      HID_TYPE_GENERIC,
      HID_STATE_READY,
  };

  SetReportSize(sizeof(pseudo_hid_report_desc));
  CheckHidReportDescriptor(pseudo_hid_report_desc);
  CheckHidInfo(expected, *hid_get_info(0));
}

}  // namespace anonymous
