// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// SMP Legacy pairing (peripheral side) — Just Works with passkey == 0,
// Passkey Entry with a build-time-fixed 6-digit passkey otherwise. Plus
// Phase 3 key distribution.

#ifndef __ble_smp_h__
#define __ble_smp_h__

#include <stdbool.h>
#include <stdint.h>

enum {
  // opcodes (BT spec Vol 3 Part H sec. 3.3)
  SMP_OP_PAIRING_REQUEST       = 0x01,
  SMP_OP_PAIRING_RESPONSE      = 0x02,
  SMP_OP_PAIRING_CONFIRM       = 0x03,
  SMP_OP_PAIRING_RANDOM        = 0x04,
  SMP_OP_PAIRING_FAILED        = 0x05,
  SMP_OP_ENCRYPTION_INFO       = 0x06,
  SMP_OP_MASTER_IDENTIFICATION = 0x07,
  SMP_OP_IDENTITY_INFO         = 0x08,
  SMP_OP_IDENTITY_ADDR_INFO    = 0x09,
  SMP_OP_SIGNING_INFO          = 0x0a,
  SMP_OP_SECURITY_REQUEST      = 0x0b,

  // session state
  SMP_STATE_IDLE = 0,
  SMP_STATE_AWAITING_CONFIRM,
  SMP_STATE_AWAITING_RANDOM,
  SMP_STATE_STK_READY,
  SMP_STATE_FAILED,
};

struct smp_session {
  uint8_t state;
  uint8_t iat;
  uint8_t rat;
  uint8_t ia[6];
  uint8_t ra[6];
  uint8_t preq[7];
  uint8_t pres[7];
  uint8_t mconfirm[16];
  uint8_t srand[16];
  uint8_t mrand[16];
  uint8_t stk[16];
  uint8_t bond_ltk[16];
  uint8_t bond_ediv[2];
  uint8_t bond_rand[8];
  uint8_t bond_irk[16];
  bool bond_set;
  uint8_t phase3_index;
  uint32_t passkey;
};

// `passkey` is the 6-digit Legacy Passkey value (0..999999); 0 selects Just
// Works (TK = 0, no MITM protection). Non-zero values negotiate Passkey
// Entry with the central displaying / accepting the same number.
void smp_init(struct smp_session* s,
              const uint8_t* peer_addr_wire,
              uint8_t peer_addr_type,
              const uint8_t* own_addr_wire,
              uint8_t own_addr_type,
              uint32_t passkey);

// Returns the number of response bytes written to `out`, or 0 when no
// response is needed.
uint8_t smp_handle_pdu(struct smp_session* s,
                       const uint8_t* pdu,
                       uint8_t pdu_len,
                       uint8_t* out,
                       uint8_t out_max);

void smp_get_stk_wire(const struct smp_session* s, uint8_t* out16);

// Iterator: returns the PDU byte count written to `out`, or 0 once all
// Phase 3 PDUs have been emitted.
uint8_t smp_next_phase3_pdu(struct smp_session* s,
                            uint8_t* out,
                            uint8_t out_max);

bool smp_lookup_ltk(const struct smp_session* s,
                    const uint8_t* ediv_le,
                    const uint8_t* rand_wire,
                    uint8_t* ltk_out16);

#endif  // __ble_smp_h__
