// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __interrupt_h__
#define __interrupt_h__

enum {
  INT_NO_INT0 = 0,
  INT_NO_TMR0 = 1,
  INT_NO_INT1 = 2,
  INT_NO_TMR1 = 3,
  INT_NO_UART = 4,
  INT_NO_TMR2 = 5,
  INT_NO_SPI0 = 6,
  INT_NO_TMR3 = 7,
  INT_NO_USB = 8,
  INT_NO_ADC = 9,
  INT_NO_UART1 = 10,
  INT_NO_PWM1 = 11,
  INT_NO_GPIO = 12,
  INT_NO_WDOG = 13,
};

#endif  // __interrupt_h__