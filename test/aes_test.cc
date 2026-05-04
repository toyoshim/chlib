// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Verifies src/crypto/aes.c against:
//   - NIST FIPS 197 Appendix B   (the cipher example walked through in §B)
//   - NIST FIPS 197 Appendix C.1 (AES-128 worked example)
//   - BT Core spec Vol 6 Part C SMP c1 sample data (both inner AES calls)
//
// These mirror the four test groups in ../aes_test.py against the Python
// reference implementation, so the C port and the Python port are pinned
// to the same wire-level outputs.

#include <stdint.h>
#include <string.h>

extern "C" {
#include "crypto/aes.h"
}

#include "gtest/gtest.h"

namespace {

uint8_t hex_nibble(char c) {
  if (c >= '0' && c <= '9') {
    return (uint8_t)(c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return (uint8_t)(c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return (uint8_t)(c - 'A' + 10);
  }
  return 0;
}

void hex2bin(const char* hex, uint8_t* out, int len) {
  for (int i = 0; i < len; i++) {
    out[i] = (uint8_t)((hex_nibble(hex[i * 2]) << 4) |
                       hex_nibble(hex[i * 2 + 1]));
  }
}

TEST(Aes, Fips197AppendixB) {
  uint8_t key[16], pt[16], ct[16], expect[16], rk[AES128_ROUND_KEYS_SIZE];
  hex2bin("2b7e151628aed2a6abf7158809cf4f3c", key, 16);
  hex2bin("3243f6a8885a308d313198a2e0370734", pt, 16);
  hex2bin("3925841d02dc09fbdc118597196a0b32", expect, 16);
  aes128_key_expansion(key, rk);
  aes128_encrypt_block(rk, pt, ct);
  EXPECT_EQ(0, memcmp(ct, expect, 16));
}

TEST(Aes, Fips197AppendixC1) {
  uint8_t key[16], pt[16], ct[16], expect[16], rk[AES128_ROUND_KEYS_SIZE];
  hex2bin("000102030405060708090a0b0c0d0e0f", key, 16);
  hex2bin("00112233445566778899aabbccddeeff", pt, 16);
  hex2bin("69c4e0d86a7b0430d8cdb78070b4c55a", expect, 16);
  aes128_key_expansion(key, rk);
  aes128_encrypt_block(rk, pt, ct);
  EXPECT_EQ(0, memcmp(ct, expect, 16));
}

// In-place encrypt should leave the buffer with the ciphertext.
TEST(Aes, EncryptInPlace) {
  uint8_t key[16], buf[16], expect[16], rk[AES128_ROUND_KEYS_SIZE];
  hex2bin("000102030405060708090a0b0c0d0e0f", key, 16);
  hex2bin("00112233445566778899aabbccddeeff", buf, 16);
  hex2bin("69c4e0d86a7b0430d8cdb78070b4c55a", expect, 16);
  aes128_key_expansion(key, rk);
  aes128_encrypt_block(rk, buf, buf);
  EXPECT_EQ(0, memcmp(buf, expect, 16));
}

TEST(Aes, SmpC1Sample) {
  // BT Core Vol 6 Part C 'c1' sample: TK = 0, p1 / p2 / r given,
  // expected M_CONFIRM is two AES-ECBs of (r XOR p1) and (e1 XOR p2).
  uint8_t key[16] = {0};
  uint8_t p1[16], p2[16], r[16], expect[16];
  hex2bin("05000800000302070710000001010001", p1, 16);
  hex2bin("00000000a1a2a3a4a5a6b1b2b3b4b5b6", p2, 16);
  hex2bin("5783D52156AD6F0E6388274EC6702EE0", r, 16);
  hex2bin("1E1E3FEF878988EAD2A74DC5BEF13B86", expect, 16);

  uint8_t rk[AES128_ROUND_KEYS_SIZE];
  aes128_key_expansion(key, rk);

  uint8_t x[16];
  for (int i = 0; i < 16; i++) {
    x[i] = (uint8_t)(r[i] ^ p1[i]);
  }
  aes128_encrypt_block(rk, x, x);  // x := e1
  for (int i = 0; i < 16; i++) {
    x[i] ^= p2[i];
  }
  aes128_encrypt_block(rk, x, x);  // x := c1 output

  EXPECT_EQ(0, memcmp(x, expect, 16));
}

}  // namespace
