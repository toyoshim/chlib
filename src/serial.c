// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "serial.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

static char U4ToHex(uint8_t val) {
  if (val < 10)
    return '0' + val;
  return 'a' + val - 10;
}

struct SerialLibrary Serial;

static void s_putc(uint8_t val) {
#ifdef _NO_UART0
  val;
#else
  putchar(val);
#endif
}

static void s_printc(int16_t val, uint8_t type) {
  if (type == BIN) {
    for (int i = 0x80; i; i >>= 1)
      Serial.putc((val & i) ? '1' : '0');
  } else if (type == HEX) {
    if (val >= 16)
      Serial.putc(U4ToHex(val >> 4));
    else
      Serial.putc('0');
    Serial.putc(U4ToHex(val & 0x0f));
  } else if (type == DEC) {
    if (val < 0) {
      Serial.putc('-');
      val = -val;
    }
    if (val >= 100)
      Serial.putc(U4ToHex(val / 100));
    if (val >= 10)
      Serial.putc(U4ToHex((val % 100) / 10));
    Serial.putc(U4ToHex(val % 10));
  }
}

static void s_print(const char* val) {
  while (*val)
    Serial.putc(*val++);
}

static void s_println(const char* val) {
  Serial.print(val);
  Serial.print("\r\n");
}

static void s_printf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  bool escape = false;
  for (uint8_t i = 0; fmt[i]; ++i) {
    if (!escape) {
      if ('\n' == fmt[i])
        Serial.putc('\r');
      if ('%' == fmt[i])
        ++escape;
      else
        Serial.putc(fmt[i]);
    } else {
      // uint8_t does not seem work correctly for SDCC's va_arg.
      switch (fmt[i]) {
        case 'd':
          Serial.printc(va_arg(ap, int), DEC);
          break;
        case 'b':
          Serial.printc(va_arg(ap, int), BIN);
          break;
        case 'x':
          Serial.printc(va_arg(ap, int), HEX);
          break;
        case 's':
          Serial.print(va_arg(ap, char*));
          break;
      }
      escape = false;
    }
  }
  va_end(ap);
}

void serial_init(void) {
  Serial.putc = s_putc;
  Serial.printc = s_printc;
  Serial.print = s_print;
  Serial.println = s_println;
  Serial.printf = s_printf;
}