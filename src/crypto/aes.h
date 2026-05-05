// Copyright 2026 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// AES-128 ECB encryption (FIPS 197).

#ifndef __aes_h__
#define __aes_h__

#include <stdint.h>

enum {
  AES128_KEY_SIZE        = 16,
  AES128_BLOCK_SIZE      = 16,
  AES128_ROUND_KEYS_SIZE = 176,
};

void aes128_key_expansion(const uint8_t key[AES128_KEY_SIZE],
                          uint8_t round_keys[AES128_ROUND_KEYS_SIZE]);

// `ciphertext` may alias `plaintext`.
void aes128_encrypt_block(const uint8_t round_keys[AES128_ROUND_KEYS_SIZE],
                          const uint8_t plaintext[AES128_BLOCK_SIZE],
                          uint8_t ciphertext[AES128_BLOCK_SIZE]);

#endif  // __aes_h__
