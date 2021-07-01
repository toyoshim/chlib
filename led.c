// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "led.h"

#include "ch559.h"
#include "timer3.h"

#define ON HIGH
#define OFF LOW
#define END 0xff

static int8_t port = -1;
static int8_t pin = -1;
static uint8_t mode = 0;
static uint8_t shot = END;
static uint16_t shot_start_time = 0;
static uint8_t polarity = LOW;

static const uint8_t patterns[][8] = {
  {  ON,  ON,  ON,  ON,  ON,  ON,  ON,  ON },  // ON
  {  ON,  ON,  ON,  ON, OFF, OFF, OFF, OFF },  // Blink
  {  ON,  ON, OFF, OFF,  ON,  ON, OFF, OFF },  // Fast Blink
  {  ON, OFF,  ON, OFF,  ON, OFF,  ON, OFF },  // Faster Blink
  {  ON, OFF,  ON, OFF, OFF, OFF, OFF, OFF },  // Blink Twice
  {  ON, OFF,  ON, OFF,  ON, OFF, OFF, OFF },  // Blink Three times
  { OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF },  // OFF
};

static const uint8_t shots[][20] = {
  { OFF,  ON, OFF, END, END, END, END, END, END, END, END, END, END, END, END, END, END, END, END, END }, // Pulse x1
  { OFF,  ON, OFF,  ON, OFF, END, END, END, END, END, END, END, END, END, END, END, END, END, END, END }, // Pulse x2
  { OFF,  ON, OFF,  ON, OFF,  ON, OFF, END, END, END, END, END, END, END, END, END, END, END, END, END }, // Pulse x3

};

void led_init(uint8_t new_port, uint8_t new_pin, uint8_t new_polarity) {
  port = new_port;
  pin = new_pin;
  mode = 0;
  polarity = new_polarity;
  pinMode(port, pin, OUTPUT);
  timer3_tick_init();
}

void led_mode(uint8_t new_mode) {
  mode = new_mode & 7;
}

void led_oneshot(uint8_t new_shot) {
  shot = new_shot % 3;
  shot_start_time = timer3_tick_msec();
}

void led_poll() {
  if (shot != END) {
    uint16_t phase = (timer3_tick_msec() - shot_start_time) / 50;
    if (patterns[shot][phase] == END) {
      shot = END;
    } else {
      digitalWrite(port, pin, shots[shot][phase] == polarity ? HIGH : LOW);
      return;
    }
  }
  uint16_t phase = timer3_tick_msec() / 125;
  digitalWrite(port, pin, patterns[mode][phase] == polarity ? HIGH : LOW);
}
