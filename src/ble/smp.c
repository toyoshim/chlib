// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "smp.h"

#include "../crypto/aes.h"
#include "../timer3.h"

// Session fields ia/ra/preq/pres/mconfirm/srand/mrand/stk are stored in
// spec/MSB order (the c1 / s1 input form). Wire bytes are reversed at the
// boundary. bond_ltk/ediv/rand/irk are stored in WIRE order to feed HCI
// LE_LTK_Request_Reply and SMP key distribution PDUs as-is.

enum {
  // Pairing Failed reasons
  SMP_PF_PASSKEY_FAILED        = 0x01,
  SMP_PF_OOB_NOT_AVAILABLE     = 0x02,
  SMP_PF_AUTH_REQUIREMENTS     = 0x03,
  SMP_PF_CONFIRM_VALUE_FAILED  = 0x04,
  SMP_PF_PAIRING_NOT_SUPPORTED = 0x05,
  SMP_PF_ENCRYPTION_KEY_SIZE   = 0x06,
  SMP_PF_COMMAND_NOT_SUPPORTED = 0x07,
  SMP_PF_UNSPECIFIED           = 0x08,

  // IO Capabilities
  SMP_IO_DISPLAY_ONLY     = 0x00,
  SMP_IO_DISPLAY_YES_NO   = 0x01,
  SMP_IO_KEYBOARD_ONLY    = 0x02,
  SMP_IO_NO_INPUT_OUTPUT  = 0x03,
  SMP_IO_KEYBOARD_DISPLAY = 0x04,

  // OOB Authentication data flag
  SMP_OOB_NOT_PRESENT = 0x00,
  SMP_OOB_PRESENT     = 0x01,

  // AuthReq bits
  SMP_AUTH_BONDING = 1 << 0,
  SMP_AUTH_MITM    = 1 << 2,
  SMP_AUTH_SC      = 1 << 3,

  // Key Distribution bits
  SMP_KD_ENC_KEY  = 1 << 0,
  SMP_KD_ID_KEY   = 1 << 1,
  SMP_KD_SIGN_KEY = 1 << 2,
  SMP_KD_LINK_KEY = 1 << 3,

  // odd, coprime with 256 → full 8-bit period for the random_counter LCG.
  RANDOM_COUNTER_STRIDE = 0x37,
};

// TK = 0 for Just Works; round keys are precomputed once on first use.
static uint8_t tk_round_keys[AES128_ROUND_KEYS_SIZE];
static bool tk_round_keys_ready;

// Stretches timer3's running tick across successive random_fill bytes so the
// STK / bond keys differ between sessions. Just Works has no secrecy goal.
static uint8_t random_counter;

// SMP PDU on-the-wire layouts (BT spec Vol 3 Part H sec 3.5 / 3.6).
struct smp_pkt_pairing {
  uint8_t opcode;
  uint8_t io_capability;
  uint8_t oob;
  uint8_t auth_req;
  uint8_t max_enc_key_size;
  uint8_t initiator_kd;
  uint8_t responder_kd;
};

// Shared by Pairing Confirm / Pairing Random / Encryption Info / Identity Info.
struct smp_pkt_value16 {
  uint8_t opcode;
  uint8_t value[16];
};

struct smp_pkt_master_ident {
  uint8_t opcode;
  uint8_t ediv[2];
  uint8_t rand[8];
};

struct smp_pkt_identity_addr {
  uint8_t opcode;
  uint8_t addr_type;
  uint8_t bd_addr[6];
};

struct smp_pkt_failed {
  uint8_t opcode;
  uint8_t reason;
};

static void ensure_tk_keys(void) {
  if (tk_round_keys_ready) {
    return;
  }
  uint8_t tk[AES128_KEY_SIZE];
  for (uint8_t i = 0; i < AES128_KEY_SIZE; i++) {
    tk[i] = 0;
  }
  aes128_key_expansion(tk, tk_round_keys);
  tk_round_keys_ready = true;
}

static void random_fill(uint8_t* buf, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    uint16_t t = timer3_tick_raw();
    buf[i] = (uint8_t)(t ^ (t >> 8) ^ random_counter);
    random_counter += RANDOM_COUNTER_STRIDE;
  }
}

static void reverse6(const uint8_t* in, uint8_t* out) {
  for (uint8_t i = 0; i < 6; i++) {
    out[i] = in[5 - i];
  }
}

// c1: confirm value (BT spec Vol 3 Part H sec 2.2.3). All values MSB.
//   p1 = pres || preq || (rat || iat)
//   p2 = padding(4) || ia || ra
//   c1 = e(k, e(k, r XOR p1) XOR p2)
static void c1(const struct smp_session* s,
               const uint8_t* r_msb,
               uint8_t* out_msb) {
  // Build p1 directly into out_msb, then XOR with r in place.
  for (uint8_t i = 0; i < sizeof(s->pres); i++) {
    out_msb[i] = s->pres[i];
    out_msb[sizeof(s->pres) + i] = s->preq[i];
  }
  out_msb[AES128_BLOCK_SIZE - 2] = s->rat;
  out_msb[AES128_BLOCK_SIZE - 1] = s->iat;
  for (uint8_t i = 0; i < AES128_BLOCK_SIZE; i++) {
    out_msb[i] ^= r_msb[i];
  }
  aes128_encrypt_block(tk_round_keys, out_msb, out_msb);
  // p2 = padding(4) || ia(6) || ra(6); needs its own buffer because out_msb
  // still holds the AES result we will XOR into.
  uint8_t p2[AES128_BLOCK_SIZE];
  for (uint8_t i = 0; i < 4; i++) {
    p2[i] = 0;
  }
  for (uint8_t i = 0; i < sizeof(s->ia); i++) {
    p2[4 + i] = s->ia[i];
    p2[4 + sizeof(s->ia) + i] = s->ra[i];
  }
  for (uint8_t i = 0; i < AES128_BLOCK_SIZE; i++) {
    out_msb[i] ^= p2[i];
  }
  aes128_encrypt_block(tk_round_keys, out_msb, out_msb);
}

// s1: STK derivation (BT spec Vol 3 Part H sec 2.2.4).
//   X = r1[lower 64] || r2[lower 64], stk = e(k, X)
static void s1(const uint8_t* r1_msb,
               const uint8_t* r2_msb,
               uint8_t* out_msb) {
  for (uint8_t i = 0; i < AES128_BLOCK_SIZE / 2; i++) {
    out_msb[i] = r1_msb[AES128_BLOCK_SIZE / 2 + i];
    out_msb[AES128_BLOCK_SIZE / 2 + i] = r2_msb[AES128_BLOCK_SIZE / 2 + i];
  }
  aes128_encrypt_block(tk_round_keys, out_msb, out_msb);
}

void smp_init(struct smp_session* s,
              const uint8_t* peer_addr_wire,
              uint8_t peer_addr_type,
              const uint8_t* own_addr_wire,
              uint8_t own_addr_type) {
  s->state = SMP_STATE_IDLE;
  s->iat = peer_addr_type & 1;
  s->rat = own_addr_type & 1;
  reverse6(peer_addr_wire, s->ia);
  reverse6(own_addr_wire, s->ra);
  s->bond_set = false;
  s->phase3_index = 0;
}

static uint8_t fail(uint8_t* out, uint8_t reason) {
  struct smp_pkt_failed* p = (struct smp_pkt_failed*)out;
  p->opcode = SMP_OP_PAIRING_FAILED;
  p->reason = reason;
  return sizeof(*p);
}

static uint8_t on_pairing_request(struct smp_session* s,
                                  const uint8_t* pdu,
                                  uint8_t pdu_len,
                                  uint8_t* out) {
  if (pdu_len != sizeof(struct smp_pkt_pairing)) {
    return fail(out, SMP_PF_UNSPECIFIED);
  }
  // preq spec/MSB = wire reversed.
  for (uint8_t i = 0; i < sizeof(s->preq); i++) {
    s->preq[i] = pdu[sizeof(s->preq) - 1 - i];
  }
  struct smp_pkt_pairing* rsp = (struct smp_pkt_pairing*)out;
  rsp->opcode = SMP_OP_PAIRING_RESPONSE;
  rsp->io_capability = SMP_IO_NO_INPUT_OUTPUT;
  rsp->oob = SMP_OOB_NOT_PRESENT;
  rsp->auth_req = SMP_AUTH_BONDING;
  rsp->max_enc_key_size = AES128_KEY_SIZE;
  rsp->initiator_kd = 0;
  rsp->responder_kd = SMP_KD_ENC_KEY;
  for (uint8_t i = 0; i < sizeof(s->pres); i++) {
    s->pres[i] = out[sizeof(s->pres) - 1 - i];
  }
  s->state = SMP_STATE_AWAITING_CONFIRM;
  return sizeof(*rsp);
}

static uint8_t on_pairing_confirm(struct smp_session* s,
                                  const uint8_t* pdu,
                                  uint8_t pdu_len,
                                  uint8_t* out) {
  if (pdu_len != sizeof(struct smp_pkt_value16) ||
      s->state != SMP_STATE_AWAITING_CONFIRM) {
    return fail(out, SMP_PF_UNSPECIFIED);
  }
  // mconfirm spec/MSB = wire reversed.
  const struct smp_pkt_value16* req = (const struct smp_pkt_value16*)pdu;
  for (uint8_t i = 0; i < sizeof(s->mconfirm); i++) {
    s->mconfirm[i] = req->value[sizeof(req->value) - 1 - i];
  }
  ensure_tk_keys();
  random_fill(s->srand, sizeof(s->srand));
  // Compute c1 into rsp->value (MSB) then reverse in place to wire order.
  struct smp_pkt_value16* rsp = (struct smp_pkt_value16*)out;
  c1(s, s->srand, rsp->value);
  for (uint8_t i = 0; i < sizeof(rsp->value) / 2; i++) {
    uint8_t t = rsp->value[i];
    rsp->value[i] = rsp->value[sizeof(rsp->value) - 1 - i];
    rsp->value[sizeof(rsp->value) - 1 - i] = t;
  }
  rsp->opcode = SMP_OP_PAIRING_CONFIRM;
  s->state = SMP_STATE_AWAITING_RANDOM;
  return sizeof(*rsp);
}

static uint8_t on_pairing_random(struct smp_session* s,
                                 const uint8_t* pdu,
                                 uint8_t pdu_len,
                                 uint8_t* out) {
  if (pdu_len != sizeof(struct smp_pkt_value16) ||
      s->state != SMP_STATE_AWAITING_RANDOM) {
    return fail(out, SMP_PF_UNSPECIFIED);
  }
  // mrand spec/MSB = wire reversed.
  const struct smp_pkt_value16* req = (const struct smp_pkt_value16*)pdu;
  for (uint8_t i = 0; i < sizeof(s->mrand); i++) {
    s->mrand[i] = req->value[sizeof(req->value) - 1 - i];
  }
  uint8_t check[16];
  c1(s, s->mrand, check);
  for (uint8_t i = 0; i < sizeof(check); i++) {
    if (check[i] != s->mconfirm[i]) {
      s->state = SMP_STATE_FAILED;
      return fail(out, SMP_PF_CONFIRM_VALUE_FAILED);
    }
  }
  s1(s->srand, s->mrand, s->stk);
  struct smp_pkt_value16* rsp = (struct smp_pkt_value16*)out;
  rsp->opcode = SMP_OP_PAIRING_RANDOM;
  for (uint8_t i = 0; i < sizeof(rsp->value); i++) {
    rsp->value[i] = s->srand[sizeof(s->srand) - 1 - i];
  }
  s->state = SMP_STATE_STK_READY;
  return sizeof(*rsp);
}

uint8_t smp_handle_pdu(struct smp_session* s,
                       const uint8_t* pdu,
                       uint8_t pdu_len,
                       uint8_t* out,
                       uint8_t out_max) {
  if (pdu_len == 0 || out_max < sizeof(struct smp_pkt_failed)) {
    return 0;
  }
  switch (pdu[0]) {
    case SMP_OP_PAIRING_REQUEST:
      return on_pairing_request(s, pdu, pdu_len, out);
    case SMP_OP_PAIRING_CONFIRM:
      return on_pairing_confirm(s, pdu, pdu_len, out);
    case SMP_OP_PAIRING_RANDOM:
      return on_pairing_random(s, pdu, pdu_len, out);
    case SMP_OP_PAIRING_FAILED:
      s->state = SMP_STATE_FAILED;
      return 0;
    case SMP_OP_ENCRYPTION_INFO:
    case SMP_OP_MASTER_IDENTIFICATION:
    case SMP_OP_IDENTITY_INFO:
    case SMP_OP_IDENTITY_ADDR_INFO:
    case SMP_OP_SIGNING_INFO:
      // Accept silently; we did not request initiator keys.
      return 0;
    default:
      return fail(out, SMP_PF_COMMAND_NOT_SUPPORTED);
  }
}

void smp_get_stk_wire(const struct smp_session* s, uint8_t* out16) {
  for (uint8_t i = 0; i < sizeof(s->stk); i++) {
    out16[i] = s->stk[sizeof(s->stk) - 1 - i];
  }
}

uint8_t smp_next_phase3_pdu(struct smp_session* s,
                            uint8_t* out,
                            uint8_t out_max) {
  if (!s->bond_set) {
    random_fill(s->bond_ltk, sizeof(s->bond_ltk));
    random_fill(s->bond_ediv, sizeof(s->bond_ediv));
    random_fill(s->bond_rand, sizeof(s->bond_rand));
    random_fill(s->bond_irk, sizeof(s->bond_irk));
    s->bond_set = true;
  }
  if (s->phase3_index == 0 && out_max >= sizeof(struct smp_pkt_value16)) {
    struct smp_pkt_value16* p = (struct smp_pkt_value16*)out;
    p->opcode = SMP_OP_ENCRYPTION_INFO;
    for (uint8_t i = 0; i < sizeof(p->value); i++) {
      p->value[i] = s->bond_ltk[i];
    }
    s->phase3_index = 1;
    return sizeof(*p);
  }
  if (s->phase3_index == 1 && out_max >= sizeof(struct smp_pkt_master_ident)) {
    struct smp_pkt_master_ident* p = (struct smp_pkt_master_ident*)out;
    p->opcode = SMP_OP_MASTER_IDENTIFICATION;
    for (uint8_t i = 0; i < sizeof(p->ediv); i++) {
      p->ediv[i] = s->bond_ediv[i];
    }
    for (uint8_t i = 0; i < sizeof(p->rand); i++) {
      p->rand[i] = s->bond_rand[i];
    }
    s->phase3_index = 2;
    return sizeof(*p);
  }
  if (s->phase3_index == 2 && out_max >= sizeof(struct smp_pkt_value16)) {
    struct smp_pkt_value16* p = (struct smp_pkt_value16*)out;
    p->opcode = SMP_OP_IDENTITY_INFO;
    for (uint8_t i = 0; i < sizeof(p->value); i++) {
      p->value[i] = s->bond_irk[i];
    }
    s->phase3_index = 3;
    return sizeof(*p);
  }
  if (s->phase3_index == 3 && out_max >= sizeof(struct smp_pkt_identity_addr)) {
    struct smp_pkt_identity_addr* p = (struct smp_pkt_identity_addr*)out;
    p->opcode = SMP_OP_IDENTITY_ADDR_INFO;
    p->addr_type = s->rat & 1;
    // ra is stored MSB; reverse to wire.
    for (uint8_t i = 0; i < sizeof(p->bd_addr); i++) {
      p->bd_addr[i] = s->ra[sizeof(p->bd_addr) - 1 - i];
    }
    s->phase3_index = 4;
    return sizeof(*p);
  }
  return 0;
}

bool smp_lookup_ltk(const struct smp_session* s,
                    const uint8_t* ediv_le,
                    const uint8_t* rand_wire,
                    uint8_t* ltk_out16) {
  if (!s->bond_set) {
    return false;
  }
  for (uint8_t i = 0; i < sizeof(s->bond_ediv); i++) {
    if (ediv_le[i] != s->bond_ediv[i]) {
      return false;
    }
  }
  for (uint8_t i = 0; i < sizeof(s->bond_rand); i++) {
    if (rand_wire[i] != s->bond_rand[i]) {
      return false;
    }
  }
  for (uint8_t i = 0; i < sizeof(s->bond_ltk); i++) {
    ltk_out16[i] = s->bond_ltk[i];
  }
  return true;
}
