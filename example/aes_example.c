// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ch559.h"
#include "crypto/aes.h"
#include "serial.h"

// NIST FIPS 197 Appendix C.1 (AES-128 worked example).
static const uint8_t key[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};
static const uint8_t plaintext[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
};
static const uint8_t expected[16] = {
    0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
    0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a,
};

static void dump16(const char* tag, const uint8_t* b) {
  Serial.print(tag);
  for (uint8_t i = 0; i < 16; i++) {
    Serial.printc(b[i], HEX);
  }
  Serial.println("");
}

// Kept out of main()'s auto/iram-stack frame: 176 + 16 bytes would otherwise
// blow the 256-byte iram stack (CH559 has no xstack with the default build
// flags). Module-level static arrays land in xdata (6 KB) and cost zero
// stack.
static uint8_t round_keys[AES128_ROUND_KEYS_SIZE];
static uint8_t ciphertext[16];

void main(void) {
  initialize();
  Serial.println("AES-128 Example (FIPS 197 C.1)");

  aes128_key_expansion(key, round_keys);
  aes128_encrypt_block(round_keys, plaintext, ciphertext);

  dump16("key:    ", key);
  dump16("pt:     ", plaintext);
  dump16("ct:     ", ciphertext);
  dump16("expect: ", expected);

  bool ok = true;
  for (uint8_t i = 0; i < 16; i++) {
    if (ciphertext[i] != expected[i]) {
      ok = false;
      break;
    }
  }
  Serial.println(ok ? "PASS" : "FAIL");

  for (;;)
    ;
}
