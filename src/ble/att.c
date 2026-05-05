// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "att.h"

enum {
  // ATT opcodes used internally (BT spec Vol 3 Part F sec. 3.4)
  ATT_OP_ERROR_RSP              = 0x01,
  ATT_OP_FIND_INFO_REQ          = 0x04,
  ATT_OP_FIND_INFO_RSP          = 0x05,
  ATT_OP_FIND_BY_TYPE_REQ       = 0x06,
  ATT_OP_FIND_BY_TYPE_RSP       = 0x07,
  ATT_OP_READ_BY_TYPE_REQ       = 0x08,
  ATT_OP_READ_BY_TYPE_RSP       = 0x09,
  ATT_OP_READ_REQ               = 0x0a,
  ATT_OP_READ_RSP               = 0x0b,
  ATT_OP_READ_BLOB_REQ          = 0x0c,
  ATT_OP_READ_BLOB_RSP          = 0x0d,
  ATT_OP_READ_BY_GROUP_TYPE_REQ = 0x10,
  ATT_OP_READ_BY_GROUP_TYPE_RSP = 0x11,
  ATT_OP_WRITE_REQ              = 0x12,
  ATT_OP_WRITE_RSP              = 0x13,
  ATT_OP_WRITE_CMD              = 0x52,

  // error codes
  ATT_ERR_INVALID_HANDLE          = 0x01,
  ATT_ERR_READ_NOT_PERMITTED      = 0x02,
  ATT_ERR_WRITE_NOT_PERMITTED     = 0x03,
  ATT_ERR_INVALID_PDU             = 0x04,
  ATT_ERR_INSUFFICIENT_AUTH       = 0x05,
  ATT_ERR_REQUEST_NOT_SUPPORTED   = 0x06,
  ATT_ERR_INVALID_OFFSET          = 0x07,
  ATT_ERR_ATTRIBUTE_NOT_FOUND     = 0x0a,
  ATT_ERR_ATTR_NOT_LONG           = 0x0b,
  ATT_ERR_INSUFFICIENT_ENCRYPTION = 0x0f,

  // Group-type UUIDs walked by group_end / find_by_type_value
  UUID_SECONDARY_SERVICE = 0x2801,
  UUID_INCLUDE           = 0x2802,

  // Find Information Response format (BT spec Vol 3 Part F sec. 3.4.3.2)
  ATT_FIND_INFO_FORMAT_UUID16 = 0x01,
};

struct att_error_rsp {
  uint8_t opcode;
  uint8_t req_op;
  uint16_t handle;
  uint8_t code;
};

struct att_exchange_mtu {
  uint8_t opcode;
  uint16_t mtu;
};

struct att_handle_range_req {
  uint8_t opcode;
  uint16_t start;
  uint16_t end;
};

struct att_typed_range_req {
  uint8_t opcode;
  uint16_t start;
  uint16_t end;
  uint16_t type;
};

struct att_handle_req {
  uint8_t opcode;
  uint16_t handle;
};

struct att_handle_offset_req {
  uint8_t opcode;
  uint16_t handle;
  uint16_t offset;
};

struct att_read_by_type_rsp {
  uint8_t opcode;
  uint8_t length;
};

struct att_find_info_rsp {
  uint8_t opcode;
  uint8_t format;
};

static uint8_t err_rsp(uint8_t* out,
                       uint8_t req_op,
                       uint16_t handle,
                       uint8_t code) {
  struct att_error_rsp* rsp = (struct att_error_rsp*)out;
  rsp->opcode = ATT_OP_ERROR_RSP;
  rsp->req_op = req_op;
  rsp->handle = handle;
  rsp->code = code;
  return sizeof(*rsp);
}

static const struct ble_attr* find_attr(const struct att_ctx* ctx,
                                        uint16_t handle) {
  for (uint8_t i = 0; i < ctx->db_count; i++) {
    if (ctx->db[i].handle == handle) {
      return &ctx->db[i];
    }
  }
  return 0;
}

static uint16_t group_end(const struct att_ctx* ctx, uint8_t start_idx) {
  for (uint8_t i = start_idx + 1; i < ctx->db_count; i++) {
    uint16_t t = ctx->db[i].type_uuid;
    if (t == UUID_PRIMARY_SERVICE || t == UUID_SECONDARY_SERVICE) {
      return ctx->db[i - 1].handle;
    }
  }
  return ctx->db[ctx->db_count - 1].handle;
}

static uint8_t handle_exchange_mtu(const struct att_ctx* ctx,
                                   const uint8_t* req,
                                   uint8_t req_len,
                                   uint8_t* out) {
  if (req_len != sizeof(struct att_exchange_mtu)) {
    return err_rsp(out, req[0], 0, ATT_ERR_INVALID_PDU);
  }
  struct att_exchange_mtu* rsp = (struct att_exchange_mtu*)out;
  rsp->opcode = ATT_OP_EXCHANGE_MTU_RSP;
  rsp->mtu = ctx->mtu;
  return sizeof(*rsp);
}

static uint8_t handle_read_by_group_type(const struct att_ctx* ctx,
                                         const uint8_t* req,
                                         uint8_t req_len,
                                         uint8_t* out,
                                         uint8_t out_max) {
  if (req_len != sizeof(struct att_typed_range_req) && req_len != 21) {
    return err_rsp(out, req[0], 0, ATT_ERR_INVALID_PDU);
  }
  const struct att_typed_range_req* r = (const struct att_typed_range_req*)req;
  if (r->start == 0 || r->start > r->end) {
    return err_rsp(out, req[0], r->start, ATT_ERR_INVALID_HANDLE);
  }
  if (req_len != sizeof(*r)) {
    return err_rsp(out, req[0], r->start, ATT_ERR_ATTRIBUTE_NOT_FOUND);
  }
  if (r->type != UUID_PRIMARY_SERVICE && r->type != UUID_SECONDARY_SERVICE) {
    return err_rsp(out, req[0], r->start, ATT_ERR_ATTRIBUTE_NOT_FOUND);
  }
  struct att_read_by_type_rsp* rsp = (struct att_read_by_type_rsp*)out;
  uint8_t rec_len = 0;
  uint8_t pos = sizeof(*rsp);
  for (uint8_t i = 0; i < ctx->db_count; i++) {
    if (ctx->db[i].type_uuid != r->type) {
      continue;
    }
    if (ctx->db[i].handle < r->start || ctx->db[i].handle > r->end) {
      continue;
    }
    uint8_t this_len = 4 + ctx->db[i].value_len;
    if (rec_len == 0) {
      rec_len = this_len;
    } else if (this_len != rec_len) {
      break;
    }
    if (pos + rec_len > out_max || pos + rec_len > ctx->mtu) {
      break;
    }
    *(uint16_t*)(out + pos) = ctx->db[i].handle;
    *(uint16_t*)(out + pos + 2) = group_end(ctx, i);
    for (uint8_t k = 0; k < ctx->db[i].value_len; k++) {
      out[pos + 4 + k] = ctx->db[i].value[k];
    }
    pos += rec_len;
  }
  if (pos == sizeof(*rsp)) {
    return err_rsp(out, req[0], r->start, ATT_ERR_ATTRIBUTE_NOT_FOUND);
  }
  rsp->opcode = ATT_OP_READ_BY_GROUP_TYPE_RSP;
  rsp->length = rec_len;
  return pos;
}

static uint8_t handle_read_by_type(const struct att_ctx* ctx,
                                   const uint8_t* req,
                                   uint8_t req_len,
                                   uint8_t* out,
                                   uint8_t out_max) {
  if (req_len != sizeof(struct att_typed_range_req) && req_len != 21) {
    return err_rsp(out, req[0], 0, ATT_ERR_INVALID_PDU);
  }
  const struct att_typed_range_req* r = (const struct att_typed_range_req*)req;
  if (r->start == 0 || r->start > r->end) {
    return err_rsp(out, req[0], r->start, ATT_ERR_INVALID_HANDLE);
  }
  if (req_len != sizeof(*r)) {
    return err_rsp(out, req[0], r->start, ATT_ERR_ATTRIBUTE_NOT_FOUND);
  }
  struct att_read_by_type_rsp* rsp = (struct att_read_by_type_rsp*)out;
  uint8_t rec_len = 0;
  uint8_t pos = sizeof(*rsp);
  for (uint8_t i = 0; i < ctx->db_count; i++) {
    if (ctx->db[i].type_uuid != r->type) {
      continue;
    }
    if (ctx->db[i].handle < r->start || ctx->db[i].handle > r->end) {
      continue;
    }
    uint8_t this_len = 2 + ctx->db[i].value_len;
    if (rec_len == 0) {
      rec_len = this_len;
    } else if (this_len != rec_len) {
      break;
    }
    if (pos + rec_len > out_max || pos + rec_len > ctx->mtu) {
      break;
    }
    *(uint16_t*)(out + pos) = ctx->db[i].handle;
    for (uint8_t k = 0; k < ctx->db[i].value_len; k++) {
      out[pos + 2 + k] = ctx->db[i].value[k];
    }
    pos += rec_len;
  }
  if (pos == sizeof(*rsp)) {
    return err_rsp(out, req[0], r->start, ATT_ERR_ATTRIBUTE_NOT_FOUND);
  }
  rsp->opcode = ATT_OP_READ_BY_TYPE_RSP;
  rsp->length = rec_len;
  return pos;
}

static uint8_t handle_find_info(const struct att_ctx* ctx,
                                const uint8_t* req,
                                uint8_t req_len,
                                uint8_t* out,
                                uint8_t out_max) {
  if (req_len != sizeof(struct att_handle_range_req)) {
    return err_rsp(out, req[0], 0, ATT_ERR_INVALID_PDU);
  }
  const struct att_handle_range_req* r =
      (const struct att_handle_range_req*)req;
  if (r->start == 0 || r->start > r->end) {
    return err_rsp(out, req[0], r->start, ATT_ERR_INVALID_HANDLE);
  }
  struct att_find_info_rsp* rsp = (struct att_find_info_rsp*)out;
  uint8_t pos = sizeof(*rsp);
  for (uint8_t i = 0; i < ctx->db_count; i++) {
    if (ctx->db[i].handle < r->start || ctx->db[i].handle > r->end) {
      continue;
    }
    if (pos + 4 > out_max || pos + 4 > ctx->mtu) {
      break;
    }
    *(uint16_t*)(out + pos) = ctx->db[i].handle;
    *(uint16_t*)(out + pos + 2) = ctx->db[i].type_uuid;
    pos += 4;
  }
  if (pos == sizeof(*rsp)) {
    return err_rsp(out, req[0], r->start, ATT_ERR_ATTRIBUTE_NOT_FOUND);
  }
  rsp->opcode = ATT_OP_FIND_INFO_RSP;
  rsp->format = ATT_FIND_INFO_FORMAT_UUID16;
  return pos;
}

static uint8_t handle_find_by_type_value(const struct att_ctx* ctx,
                                         const uint8_t* req,
                                         uint8_t req_len,
                                         uint8_t* out,
                                         uint8_t out_max) {
  if (req_len < sizeof(struct att_typed_range_req)) {
    return err_rsp(out, req[0], 0, ATT_ERR_INVALID_PDU);
  }
  const struct att_typed_range_req* r = (const struct att_typed_range_req*)req;
  if (r->start == 0 || r->start > r->end) {
    return err_rsp(out, req[0], r->start, ATT_ERR_INVALID_HANDLE);
  }
  const uint8_t* value = req + sizeof(*r);
  uint8_t value_len = req_len - sizeof(*r);
  uint8_t pos = 1;  // Response is just opcode + records (no length byte).
  for (uint8_t i = 0; i < ctx->db_count; i++) {
    if (ctx->db[i].type_uuid != r->type) {
      continue;
    }
    if (ctx->db[i].handle < r->start || ctx->db[i].handle > r->end) {
      continue;
    }
    if (ctx->db[i].value_len != value_len) {
      continue;
    }
    bool match = true;
    for (uint8_t k = 0; k < value_len; k++) {
      if (ctx->db[i].value[k] != value[k]) {
        match = false;
        break;
      }
    }
    if (!match) {
      continue;
    }
    if (pos + 4 > out_max || pos + 4 > ctx->mtu) {
      break;
    }
    uint16_t end_h = ctx->db[i].handle;
    if (r->type == UUID_PRIMARY_SERVICE || r->type == UUID_SECONDARY_SERVICE) {
      end_h = group_end(ctx, i);
    }
    *(uint16_t*)(out + pos) = ctx->db[i].handle;
    *(uint16_t*)(out + pos + 2) = end_h;
    pos += 4;
  }
  if (pos == 1) {
    return err_rsp(out, req[0], r->start, ATT_ERR_ATTRIBUTE_NOT_FOUND);
  }
  out[0] = ATT_OP_FIND_BY_TYPE_RSP;
  return pos;
}

static uint8_t handle_read(const struct att_ctx* ctx,
                           const uint8_t* req,
                           uint8_t req_len,
                           uint8_t* out,
                           uint8_t out_max) {
  if (req_len != sizeof(struct att_handle_req)) {
    return err_rsp(out, req[0], 0, ATT_ERR_INVALID_PDU);
  }
  const struct att_handle_req* r = (const struct att_handle_req*)req;
  const struct ble_attr* a = find_attr(ctx, r->handle);
  if (!a) {
    return err_rsp(out, req[0], r->handle, ATT_ERR_INVALID_HANDLE);
  }
  if ((a->flags & BLE_ATTR_ENCRYPTED) && !ctx->encrypted) {
    return err_rsp(out, req[0], r->handle, ATT_ERR_INSUFFICIENT_ENCRYPTION);
  }
  out[0] = ATT_OP_READ_RSP;
  uint8_t cap = out_max - 1;
  if (ctx->mtu - 1 < cap) {
    cap = ctx->mtu - 1;
  }
  uint8_t n = a->value_len < cap ? a->value_len : cap;
  for (uint8_t i = 0; i < n; i++) {
    out[1 + i] = a->value[i];
  }
  return 1 + n;
}

static uint8_t handle_read_blob(const struct att_ctx* ctx,
                                const uint8_t* req,
                                uint8_t req_len,
                                uint8_t* out,
                                uint8_t out_max) {
  if (req_len != sizeof(struct att_handle_offset_req)) {
    return err_rsp(out, req[0], 0, ATT_ERR_INVALID_PDU);
  }
  const struct att_handle_offset_req* r =
      (const struct att_handle_offset_req*)req;
  const struct ble_attr* a = find_attr(ctx, r->handle);
  if (!a) {
    return err_rsp(out, req[0], r->handle, ATT_ERR_INVALID_HANDLE);
  }
  if ((a->flags & BLE_ATTR_ENCRYPTED) && !ctx->encrypted) {
    return err_rsp(out, req[0], r->handle, ATT_ERR_INSUFFICIENT_ENCRYPTION);
  }
  if (r->offset > a->value_len) {
    return err_rsp(out, req[0], r->handle, ATT_ERR_INVALID_OFFSET);
  }
  out[0] = ATT_OP_READ_BLOB_RSP;
  uint8_t cap = out_max - 1;
  if (ctx->mtu - 1 < cap) {
    cap = ctx->mtu - 1;
  }
  uint8_t avail = a->value_len - r->offset;
  uint8_t n = avail < cap ? avail : cap;
  for (uint8_t i = 0; i < n; i++) {
    out[1 + i] = a->value[r->offset + i];
  }
  return 1 + n;
}

static uint8_t do_write(const struct att_ctx* ctx,
                        uint16_t handle,
                        const uint8_t* value,
                        uint8_t value_len) {
  for (uint8_t i = 0; i < ctx->db_count; i++) {
    if (ctx->db[i].handle != handle) {
      continue;
    }
    const struct ble_attr* a = &ctx->db[i];
    if (!(a->flags & BLE_ATTR_WRITABLE)) {
      return ATT_ERR_WRITE_NOT_PERMITTED;
    }
    if ((a->flags & BLE_ATTR_ENCRYPTED) && !ctx->encrypted) {
      return ATT_ERR_INSUFFICIENT_ENCRYPTION;
    }
    if (value_len > a->value_len) {
      return ATT_ERR_INVALID_PDU;
    }
    for (uint8_t k = 0; k < value_len; k++) {
      a->value[k] = value[k];
    }
    return 0;
  }
  return ATT_ERR_INVALID_HANDLE;
}

static uint8_t handle_write_req(const struct att_ctx* ctx,
                                const uint8_t* req,
                                uint8_t req_len,
                                uint8_t* out) {
  if (req_len < sizeof(struct att_handle_req)) {
    return err_rsp(out, req[0], 0, ATT_ERR_INVALID_PDU);
  }
  const struct att_handle_req* r = (const struct att_handle_req*)req;
  uint8_t err = do_write(ctx, r->handle, req + sizeof(*r),
                         req_len - sizeof(*r));
  if (err != 0) {
    return err_rsp(out, req[0], r->handle, err);
  }
  out[0] = ATT_OP_WRITE_RSP;
  return 1;
}

uint8_t att_dispatch(const struct att_ctx* ctx,
                     const uint8_t* req,
                     uint8_t req_len,
                     uint8_t* out,
                     uint8_t out_max) {
  if (req_len == 0 || out_max < sizeof(struct att_error_rsp)) {
    return 0;
  }
  switch (req[0]) {
    case ATT_OP_EXCHANGE_MTU_REQ:
      return handle_exchange_mtu(ctx, req, req_len, out);
    case ATT_OP_READ_BY_GROUP_TYPE_REQ:
      return handle_read_by_group_type(ctx, req, req_len, out, out_max);
    case ATT_OP_READ_BY_TYPE_REQ:
      return handle_read_by_type(ctx, req, req_len, out, out_max);
    case ATT_OP_FIND_INFO_REQ:
      return handle_find_info(ctx, req, req_len, out, out_max);
    case ATT_OP_FIND_BY_TYPE_REQ:
      return handle_find_by_type_value(ctx, req, req_len, out, out_max);
    case ATT_OP_READ_REQ:
      return handle_read(ctx, req, req_len, out, out_max);
    case ATT_OP_READ_BLOB_REQ:
      return handle_read_blob(ctx, req, req_len, out, out_max);
    case ATT_OP_WRITE_REQ:
      return handle_write_req(ctx, req, req_len, out);
    case ATT_OP_WRITE_CMD:
      if (req_len >= sizeof(struct att_handle_req)) {
        const struct att_handle_req* r = (const struct att_handle_req*)req;
        do_write(ctx, r->handle, req + sizeof(*r), req_len - sizeof(*r));
      }
      return 0;
    default:
      return err_rsp(out, req[0], 0, ATT_ERR_REQUEST_NOT_SUPPORTED);
  }
}
