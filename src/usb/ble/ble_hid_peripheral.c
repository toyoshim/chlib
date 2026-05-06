// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ble_hid_peripheral.h"

#include "../../ble/hci.h"
#include "ble_peripheral.h"

enum {
  // GAP / Device Information / Battery / HID characteristic UUIDs.
  UUID_GAP_SVC           = 0x1800,
  UUID_DEV_INFO_SVC      = 0x180a,
  UUID_BATTERY_SVC       = 0x180f,
  UUID_HID_SVC           = 0x1812,

  UUID_DEVICE_NAME       = 0x2a00,
  UUID_APPEARANCE        = 0x2a01,
  UUID_MFR_NAME          = 0x2a29,
  UUID_MODEL_NUMBER      = 0x2a24,
  UUID_PNP_ID            = 0x2a50,
  UUID_BATTERY_LEVEL     = 0x2a19,
  UUID_HID_INFORMATION   = 0x2a4a,
  UUID_HID_CONTROL_POINT = 0x2a4c,
  UUID_HID_REPORT_MAP    = 0x2a4b,
  UUID_HID_REPORT        = 0x2a4d,

  // Caps on the variable parts of the static state.
  HID_REPORT_MAX  = 32,
  LOCAL_NAME_MAX  = 30,

  // Handles. Every characteristic decl carries its value handle inline, so
  // the fixed map has to agree with the entries below.
  H_GAP_PRIMARY      = 0x0001,
  H_GAP_NAME_DECL    = 0x0002,
  H_GAP_NAME         = 0x0003,
  H_GAP_APPEAR_DECL  = 0x0004,
  H_GAP_APPEAR       = 0x0005,
  H_DI_PRIMARY       = 0x0010,
  H_DI_MFR_DECL      = 0x0011,
  H_DI_MFR           = 0x0012,
  H_DI_MODEL_DECL    = 0x0013,
  H_DI_MODEL         = 0x0014,
  H_DI_PNP_DECL      = 0x0015,
  H_DI_PNP           = 0x0016,
  H_HID_PRIMARY      = 0x0020,
  H_HID_INFO_DECL    = 0x0021,
  H_HID_INFO         = 0x0022,
  H_HID_CTL_DECL     = 0x0023,
  H_HID_CTL          = 0x0024,
  H_HID_MAP_DECL     = 0x0025,
  H_HID_MAP          = 0x0026,
  H_HID_REPORT_DECL  = 0x0027,
  H_HID_REPORT       = 0x0028,
  H_HID_REPORT_CCCD  = 0x0029,
  H_HID_REPORT_REF   = 0x002a,
  // Battery handles sit after HID so the optional service can be dropped
  // by truncating db_count without holes in the HID handle range.
  H_BAT_PRIMARY      = 0x0030,
  H_BAT_LEVEL_DECL   = 0x0031,
  H_BAT_LEVEL        = 0x0032,
  H_BAT_CCCD         = 0x0033,

  BAT_ATTR_COUNT     = 4,
};

// Service Primary UUIDs and characteristic declarations are all const.
static const uint8_t v_gap_primary[]     = {UUID_GAP_SVC & 0xff,
                                            UUID_GAP_SVC >> 8};
static const uint8_t v_devinfo_primary[] = {UUID_DEV_INFO_SVC & 0xff,
                                            UUID_DEV_INFO_SVC >> 8};
static const uint8_t v_battery_primary[] = {UUID_BATTERY_SVC & 0xff,
                                            UUID_BATTERY_SVC >> 8};
static const uint8_t v_hid_primary[]     = {UUID_HID_SVC & 0xff,
                                            UUID_HID_SVC >> 8};

static const uint8_t v_char_devname[]  = {CHAR_PROP_READ, H_GAP_NAME, 0x00,
                                          UUID_DEVICE_NAME & 0xff,
                                          UUID_DEVICE_NAME >> 8};
static const uint8_t v_char_appear[]   = {CHAR_PROP_READ, H_GAP_APPEAR, 0x00,
                                          UUID_APPEARANCE & 0xff,
                                          UUID_APPEARANCE >> 8};
static const uint8_t v_char_mfr[]      = {CHAR_PROP_READ, H_DI_MFR, 0x00,
                                          UUID_MFR_NAME & 0xff,
                                          UUID_MFR_NAME >> 8};
static const uint8_t v_char_model[]    = {CHAR_PROP_READ, H_DI_MODEL, 0x00,
                                          UUID_MODEL_NUMBER & 0xff,
                                          UUID_MODEL_NUMBER >> 8};
static const uint8_t v_char_pnp[]      = {CHAR_PROP_READ, H_DI_PNP, 0x00,
                                          UUID_PNP_ID & 0xff,
                                          UUID_PNP_ID >> 8};
static const uint8_t v_char_battery[]  = {CHAR_PROP_READ | CHAR_PROP_NOTIFY,
                                          H_BAT_LEVEL, 0x00,
                                          UUID_BATTERY_LEVEL & 0xff,
                                          UUID_BATTERY_LEVEL >> 8};
static const uint8_t v_char_hid_info[] = {CHAR_PROP_READ, H_HID_INFO, 0x00,
                                          UUID_HID_INFORMATION & 0xff,
                                          UUID_HID_INFORMATION >> 8};
static const uint8_t v_char_hid_ctl[]  = {CHAR_PROP_WRITE_NO_RSP,
                                          H_HID_CTL, 0x00,
                                          UUID_HID_CONTROL_POINT & 0xff,
                                          UUID_HID_CONTROL_POINT >> 8};
static const uint8_t v_char_hid_map[]  = {CHAR_PROP_READ, H_HID_MAP, 0x00,
                                          UUID_HID_REPORT_MAP & 0xff,
                                          UUID_HID_REPORT_MAP >> 8};
static const uint8_t v_char_hid_rep[]  = {CHAR_PROP_READ | CHAR_PROP_NOTIFY,
                                          H_HID_REPORT, 0x00,
                                          UUID_HID_REPORT & 0xff,
                                          UUID_HID_REPORT >> 8};

// bcdHID = 0x0111, country code 0, flags = RemoteWake | NormallyConnectable.
static const uint8_t v_hid_info[]       = {0x11, 0x01, 0x00, 0x03};
static const uint8_t v_hid_report_ref[] = {0x00, 0x01};

// Variable / mutable values.
static uint8_t v_appearance[2];
static uint8_t v_pnp[7];
static uint8_t v_battery_level[1];
static uint8_t v_battery_cccd[2];
static uint8_t v_hid_ctl_point[1];
static uint8_t v_hid_report[HID_REPORT_MAX];
static uint8_t v_hid_cccd[2];

static struct ble_attr db[] = {
    {H_GAP_PRIMARY, UUID_PRIMARY_SERVICE,
     (uint8_t*)v_gap_primary, sizeof(v_gap_primary), 0},
    {H_GAP_NAME_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_devname, sizeof(v_char_devname), 0},
    {H_GAP_NAME, UUID_DEVICE_NAME, 0, 0, 0},
    {H_GAP_APPEAR_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_appear, sizeof(v_char_appear), 0},
    {H_GAP_APPEAR, UUID_APPEARANCE,
     v_appearance, sizeof(v_appearance), 0},

    {H_DI_PRIMARY, UUID_PRIMARY_SERVICE,
     (uint8_t*)v_devinfo_primary, sizeof(v_devinfo_primary), 0},
    {H_DI_MFR_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_mfr, sizeof(v_char_mfr), 0},
    {H_DI_MFR, UUID_MFR_NAME, 0, 0, 0},
    {H_DI_MODEL_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_model, sizeof(v_char_model), 0},
    {H_DI_MODEL, UUID_MODEL_NUMBER, 0, 0, 0},
    {H_DI_PNP_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_pnp, sizeof(v_char_pnp), 0},
    {H_DI_PNP, UUID_PNP_ID, v_pnp, sizeof(v_pnp), 0},

    {H_HID_PRIMARY, UUID_PRIMARY_SERVICE,
     (uint8_t*)v_hid_primary, sizeof(v_hid_primary), 0},
    {H_HID_INFO_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_hid_info, sizeof(v_char_hid_info), 0},
    {H_HID_INFO, UUID_HID_INFORMATION,
     (uint8_t*)v_hid_info, sizeof(v_hid_info), 0},
    {H_HID_CTL_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_hid_ctl, sizeof(v_char_hid_ctl), 0},
    {H_HID_CTL, UUID_HID_CONTROL_POINT,
     v_hid_ctl_point, sizeof(v_hid_ctl_point), BLE_ATTR_WRITABLE},
    {H_HID_MAP_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_hid_map, sizeof(v_char_hid_map), 0},
    {H_HID_MAP, UUID_HID_REPORT_MAP, 0, 0, 0},
    {H_HID_REPORT_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_hid_rep, sizeof(v_char_hid_rep), 0},
    {H_HID_REPORT, UUID_HID_REPORT,
     v_hid_report, 0, BLE_ATTR_ENCRYPTED},
    {H_HID_REPORT_CCCD, UUID_CCCD,
     v_hid_cccd, sizeof(v_hid_cccd),
     BLE_ATTR_WRITABLE | BLE_ATTR_ENCRYPTED},
    {H_HID_REPORT_REF, UUID_REPORT_REFERENCE,
     (uint8_t*)v_hid_report_ref, sizeof(v_hid_report_ref), 0},

    // Battery Service (optional). Kept at the end of the table so it can
    // be excluded by truncating db_count when the caller leaves the
    // battery_level callback null.
    {H_BAT_PRIMARY, UUID_PRIMARY_SERVICE,
     (uint8_t*)v_battery_primary, sizeof(v_battery_primary), 0},
    {H_BAT_LEVEL_DECL, UUID_CHARACTERISTIC,
     (uint8_t*)v_char_battery, sizeof(v_char_battery), 0},
    {H_BAT_LEVEL, UUID_BATTERY_LEVEL,
     v_battery_level, sizeof(v_battery_level), 0},
    {H_BAT_CCCD, UUID_CCCD,
     v_battery_cccd, sizeof(v_battery_cccd), BLE_ATTR_WRITABLE},
};

#define ATTR_COUNT (sizeof(db) / sizeof(db[0]))

// Advertising payload: Flags + Appearance + UUID16(HID, optionally Battery).
// Each AD record's length field is the count of bytes that follow it (type
// + value). uuid_battery is at the tail so it can be dropped from the
// transmitted advertisement by shortening adv_data_len when the caller
// leaves the battery_level callback null.
static struct {
  uint8_t flags_len;
  uint8_t flags_type;
  uint8_t flags_value;
  uint8_t appear_len;
  uint8_t appear_type;
  uint16_t appearance;
  uint8_t uuid_len;
  uint8_t uuid_type;
  uint16_t uuid_hid;
  uint16_t uuid_battery;
} adv_data = {
    1 + sizeof(uint8_t),
    HCI_AD_FLAGS,
    HCI_AD_FLAG_LE_GENERAL_DISC | HCI_AD_FLAG_BR_EDR_NOT_SUPPORTED,
    1 + sizeof(uint16_t),
    HCI_AD_APPEARANCE,
    0,
    1 + 2 * sizeof(uint16_t),
    HCI_AD_UUID16_COMPLETE,
    UUID_HID_SVC,
    UUID_BATTERY_SVC,
};

// Scan response: complete local name only.
static struct {
  uint8_t name_len;
  uint8_t name_type;
  uint8_t name[LOCAL_NAME_MAX];
} scan_rsp = {
    0,
    HCI_AD_LOCAL_NAME_COMPLETE,
    {0},
};
static uint8_t scan_rsp_len;

static const struct ble_hid_peripheral* hid;
static struct ble_peripheral underlying_ble;
static bool encrypted_state;
static bool notify_state;

static uint8_t cstr_len(const char* s, uint8_t cap) {
  uint8_t n = 0;
  while (n < cap && s[n] != '\0') {
    n++;
  }
  return n;
}

static void patch_attr(uint16_t handle, uint8_t* value, uint8_t value_len) {
  for (uint8_t i = 0; i < ATTR_COUNT; i++) {
    if (db[i].handle == handle) {
      db[i].value = value;
      db[i].value_len = value_len;
      return;
    }
  }
}

static void build_scan_rsp(void) {
  uint8_t name_len = cstr_len(hid->local_name, LOCAL_NAME_MAX);
  scan_rsp.name_len = 1 + name_len;
  for (uint8_t i = 0; i < name_len; i++) {
    scan_rsp.name[i] = (uint8_t)hid->local_name[i];
  }
  scan_rsp_len = 2 + name_len;
}

static void on_ble_advertising(void) {
  if (hid->ready) {
    hid->ready();
  }
}

static void on_ble_connected(uint16_t handle) {
  (void)handle;
  encrypted_state = false;
  notify_state = false;
  v_hid_cccd[0] = 0;
  v_hid_cccd[1] = 0;
  if (hid->connected) {
    hid->connected();
  }
}

static void on_ble_disconnected(void) {
  encrypted_state = false;
  notify_state = false;
  v_hid_cccd[0] = 0;
  v_hid_cccd[1] = 0;
  if (hid->disconnected) {
    hid->disconnected();
  }
}

static void on_ble_encrypted(bool on) {
  encrypted_state = on;
}

static void on_ble_attr_write(uint16_t handle,
                              const uint8_t* value,
                              uint8_t value_len) {
  if (handle != H_HID_REPORT_CCCD || value_len < 1) {
    return;
  }
  bool now = (value[0] & 0x01) != 0;
  if (now == notify_state) {
    return;
  }
  notify_state = now;
  if (hid->notify_enabled) {
    hid->notify_enabled(now);
  }
}

static void on_ble_sent(void) {
  if (hid->sent) {
    hid->sent();
  }
}

void ble_hid_peripheral_init(const struct ble_hid_peripheral* config,
                             uint8_t usb_host_flags) {
  hid = config;
  v_appearance[0] = config->appearance & 0xff;
  v_appearance[1] = config->appearance >> 8;
  v_pnp[0] = config->pnp_id.vendor_id_source;
  v_pnp[1] = config->pnp_id.vendor_id & 0xff;
  v_pnp[2] = config->pnp_id.vendor_id >> 8;
  v_pnp[3] = config->pnp_id.product_id & 0xff;
  v_pnp[4] = config->pnp_id.product_id >> 8;
  v_pnp[5] = config->pnp_id.product_version & 0xff;
  v_pnp[6] = config->pnp_id.product_version >> 8;
  for (uint8_t i = 0; i < sizeof(v_battery_cccd); i++) {
    v_battery_cccd[i] = 0;
  }
  v_hid_ctl_point[0] = 0;
  for (uint8_t i = 0; i < HID_REPORT_MAX; i++) {
    v_hid_report[i] = 0;
  }
  v_hid_cccd[0] = 0;
  v_hid_cccd[1] = 0;

  adv_data.appearance = config->appearance;
  build_scan_rsp();
  patch_attr(H_GAP_NAME, (uint8_t*)config->local_name,
             cstr_len(config->local_name, 0xff));
  patch_attr(H_DI_MFR, (uint8_t*)config->manufacturer,
             cstr_len(config->manufacturer, 0xff));
  patch_attr(H_DI_MODEL, (uint8_t*)config->model,
             cstr_len(config->model, 0xff));
  patch_attr(H_HID_MAP, (uint8_t*)config->report_map,
             config->report_map_len);
  patch_attr(H_HID_REPORT, v_hid_report, config->report_len);
  if (config->battery_level) {
    v_battery_level[0] = config->battery_level();
  }

  underlying_ble.usb_host_flags = usb_host_flags;
  underlying_ble.adv_interval = 0;
  underlying_ble.adv_data = (const uint8_t*)&adv_data;
  underlying_ble.adv_data_len = sizeof(adv_data);
  underlying_ble.scan_rsp_data = (const uint8_t*)&scan_rsp;
  underlying_ble.scan_rsp_data_len = scan_rsp_len;
  underlying_ble.passkey = config->passkey;
  underlying_ble.db = db;
  underlying_ble.db_count = ATTR_COUNT;
  if (!config->battery_level) {
    // Drop the trailing UUID16 (Battery) bytes from the AD record so the
    // service we don't expose isn't advertised either.
    adv_data.uuid_len -= sizeof(uint16_t);
    underlying_ble.adv_data_len -= sizeof(uint16_t);
    underlying_ble.db_count -= BAT_ATTR_COUNT;
  }
  underlying_ble.advertising = on_ble_advertising;
  underlying_ble.connected = on_ble_connected;
  underlying_ble.disconnected = on_ble_disconnected;
  underlying_ble.encrypted = on_ble_encrypted;
  underlying_ble.attr_write = on_ble_attr_write;
  underlying_ble.sent = on_ble_sent;

  ble_peripheral_init(&underlying_ble);
}

void ble_hid_peripheral_poll(void) {
  if (hid->battery_level) {
    v_battery_level[0] = hid->battery_level();
  }
  ble_peripheral_poll();
}

bool ble_hid_peripheral_send_report(const uint8_t* report, uint8_t len) {
  if (!encrypted_state || !notify_state || len > HID_REPORT_MAX) {
    return false;
  }
  for (uint8_t i = 0; i < len; i++) {
    v_hid_report[i] = report[i];
  }
  return ble_peripheral_send_notification(H_HID_REPORT, v_hid_report, len);
}
