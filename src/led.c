// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "led.h"

#include "gpio.h"
#include "timer3.h"

#define ON_ HIGH
#define OFF LOW
#define END 0xff

static int8_t port = -1;
static int8_t pin = -1;
static uint8_t mode = 0;
static uint8_t shot = END;
static uint16_t shot_start_time = 0;
static uint8_t shot_phase = 0;
static uint8_t polarity = LOW;

static const uint8_t patterns[][8] = {
    {ON_, ON_, ON_, ON_, ON_, ON_, ON_, ON_},  // ON
    {ON_, ON_, ON_, ON_, OFF, OFF, OFF, OFF},  // Blink
    {ON_, ON_, OFF, OFF, ON_, ON_, OFF, OFF},  // Fast Blink
    {ON_, OFF, ON_, OFF, ON_, OFF, ON_, OFF},  // Faster Blink
    {ON_, OFF, ON_, OFF, OFF, OFF, OFF, OFF},  // Blink Twice
    {ON_, OFF, ON_, OFF, ON_, OFF, OFF, OFF},  // Blink Three times
    {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF},  // OFF
};

static const uint8_t shots[][20] = {
    {OFF, ON_, OFF, END, END, END, END, END, END, END,
     END, END, END, END, END, END, END, END, END, END},  // Pulse x1
    {OFF, ON_, OFF, ON_, OFF, END, END, END, END, END,
     END, END, END, END, END, END, END, END, END, END},  // Pulse x2
    {OFF, ON_, OFF, ON_, OFF, ON_, OFF, END, END, END,
     END, END, END, END, END, END, END, END, END, END},  // Pulse x3
    {OFF, ON_, OFF, ON_, OFF, ON_, OFF, ON_, OFF, END,
     END, END, END, END, END, END, END, END, END, END},  // Pulse x4
    {OFF, ON_, OFF, ON_, OFF, ON_, OFF, ON_, OFF, ON_,
     OFF, END, END, END, END, END, END, END, END, END},  // Pulse x5
    {OFF, ON_, OFF, ON_, OFF, ON_, OFF, ON_, OFF, ON_,
     OFF, ON_, OFF, END, END, END, END, END, END, END},  // Pulse x6
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
  shot = new_shot % 6;
  shot_start_time = timer3_tick_msec();
  shot_phase = 0;
}

void led_poll(void) {
  if (shot != END) {
    if (!timer3_tick_msec_between(shot_start_time, shot_start_time + 50)) {
      shot_phase++;
      if (shot_phase >= 20 || shots[shot][shot_phase] == END) {
        shot = END;
      } else {
        shot_start_time = timer3_tick_msec();
      }
      digitalWrite(port, pin, shots[shot][shot_phase] == polarity ? HIGH : LOW);
    }
    return;
  }
  uint16_t phase = timer3_tick_msec() / 125;
  digitalWrite(port, pin, patterns[mode][phase] == polarity ? HIGH : LOW);
}
