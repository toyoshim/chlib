// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ch559.h"

#include "io.h"

void (*runBootloader)() = 0xf400;

int putchar(int c) {
  while (!TI);
  TI = 0;
  SBUF = c & 0xff;
  return c;
}

static void delayU8us(uint8_t us) {
  us;
__asm
  mov r7,dpl
loop1$:
  mov a,#8
loop2$:
  dec a 
  jnz loop2$
  nop
  nop
  dec r7
  mov a,r7
  jnz loop1$
__endasm;
}

static inline void enter_safe_mode() {
  SAFE_MOD = 0x55;
  SAFE_MOD = 0xaa;
}

static inline void leave_safe_mode() {
  SAFE_MOD = 0;
}

static char U4ToHex(uint8_t val) {
  if (val < 10)
    return '0' + val;
  return 'a' + val - 10;
}

struct SerialLibrary Serial;

static void s_putc(uint8_t val) {
  putchar(val);
}

static void s_printc(uint8_t val, uint8_t type) {
  if (type == BIN) {
    for (int i = 0x80; i; i >>= 1)
      Serial.putc((val & i) ? '1' : '0');
  } else if (type == HEX) {
    if (16 <= val)
      Serial.putc(U4ToHex(val >> 4));
    else
      Serial.putc('0');
    Serial.putc(U4ToHex(val & 0x0f));
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

static uint16_t timer3_tick = 0;
void timer3_int() __interrupt INT_NO_TMR3 __using 1 {
  timer3_tick++;
  T3_STAT |= bT3_IF_END;
}

void initialize() {
  // Clock
  // Fosc = 12MHz, Fpll = 288MHz, Fusb4x = 48MHz by PLL_CFG default
  enter_safe_mode();
  CLOCK_CFG = (CLOCK_CFG & ~MASK_SYS_CK_DIV) | 6;  // Fsys = 288MHz / 6 = 48MHz
  PLL_CFG = ((24 << 0) | (6 << 5)) & 0xff;  // PLL multiplier 24, USB clock divisor 6
  leave_safe_mode();

  // UART0 115200 TX at P0.3
  P0_DIR |= 0x08;  // Set P0.3(TXD) as output
  P0_PU |= 0x08;  // Pull-up P0.3(TXD)
  PIN_FUNC |= bUART0_PIN_X;  // RXD0/TXD0 enable P0.2/P0.3

  SM0 = 0;  // 8-bits data
  SM1 = 1;  // variable baud rate, based on timer

  TMOD |= bT1_M1;  // Timer1 mode2
  T2MOD |= bTMR_CLK | bT1_CLK;  // use original Fsys, timer1 faster clock
  PCON |= SMOD;  // fast mode
  TH1 = 230;  // 256 - Fsys(48M) / 16 / baudrate(115200)

  TR1 = 1;  // Start timer1
  TI = 1;  // Set transmit interrupt flag for the first transmit

  // GPIO
  PORT_CFG = 0x00;  // 5mA push-pull for port 0-3 by default

  // SerialLibrary
  Serial.putc = s_putc;
  Serial.printc = s_printc;
  Serial.print = s_print;
  Serial.println = s_println;

  if (RESET_KEEP) {
    RESET_KEEP = 0;
    Serial.println("bootloader");
    runBootloader();
  }
  RESET_KEEP = 1;
}

void rs485_init() {
  SER1_IER |= bIER_PIN_MOD1;  // Use XA/XB

  SER1_LCR |= bLCR_DLAB;  // Allow SER1_DIV, SER1_DLM, and SER1_DLL use
  SER1_DIV = 1;
  // { SER1_DLM, SER1_DLL } = Fsys(48M) * 2 / SER1_DIV / 16 / baudrate(115200)
  SER1_DLM = 0;
  SER1_DLL = 52;  // should be set before enabling FIFO
  SER1_LCR &= ~bLCR_DLAB;

  // no parity, stop bit 1-bit, no interrupts by default
  SER1_LCR |= bLCR_WORD_SZ0 | bLCR_WORD_SZ1;  // data length 8-bits
  SER1_MCR |= bMCR_HALF;  // enable half-duplex mode
  SER1_FCR = bFCR_FIFO_EN;  // Enable FIFO
}

void rs485_send(uint8_t val) {
  while (!(SER1_LSR & bLSR_T_FIFO_EMP));
  SER1_FIFO = val;
}

bool rs485_ready() {
  return (SER1_LSR & bLSR_DATA_RDY) != 0;
}

uint8_t rs485_recv() {
  while (!rs485_ready());
  return SER1_FIFO;
}

void pwm1_init() {
  // Use P4.3
  PIN_FUNC |= bPWM1_PIN_X;
  P4_DIR |= (1 << 3);  // Set P4.3 output
  P4_OUT &= ~(1 << 3);  // Set P4.3 low
  P4_PU &= ~(1 << 3);  // Disable P4.3 pull-up

  // Clock divisor
  PWM_CK_SE = 1;  // Fsys(48M) / 1

  // Clear FIFO and count
  PWM_CTRL &= ~bPWM_CLR_ALL;

  // Enable
  PWM_CTRL |= bPWM_OUT_EN;

  // PWM cycle = 2t, duty = 1:1
  PWM_CYCLE = 2;
  PWM_DATA = 1;
}

void pwm1_duty(uint8_t data, uint8_t cycle) {
  PWM_CYCLE = cycle;
  PWM_DATA = data;
}

void pwm1_enable(bool enable) {
  if (enable)
    PWM_CTRL |= bPWM_OUT_EN;
  else
    PWM_CTRL &= ~bPWM_OUT_EN;
}

void timer3_tick_init() {
  T3_SETUP |= bT3_EN_CK_SE;  // Enable to access divisor settings register
  T3_CK_SE_L = 0x80;  // Clock = Fsys(48M) / 48k = 1kHz
  T3_CK_SE_H = 0xbb;
  T3_SETUP &= ~bT3_EN_CK_SE;  // Disable
  T3_END_L = 0x70;
  T3_END_H = 0x3e;  // 1000 * 16
  T3_CTRL |= bT3_CLR_ALL;
  T3_CTRL &= ~bT3_CLR_ALL;
  T3_SETUP |= bT3_IE_END;  // Enable end interrupt
  T3_CTRL |= bT3_CNT_EN;  // Start counting
  T3_STAT = 0xff;
  IE_TMR3 = 1;  // Enable timer3 interrupt
  EA = 1;  // Enable interruprts
}

void timer3_tick_reset() {
  IE_TMR3 = 0;
  timer3_tick = 0;
  T3_CTRL |= bT3_CLR_ALL;
  T3_CTRL &= ~bT3_CLR_ALL;
  T3_STAT |= bT3_IF_END;
  IE_TMR3 = 1;
}

void timer3_tick_wait(uint16_t msec) {
  timer3_tick_reset();
  msec <<= 4;
  uint8_t h = msec >> 8;
  uint8_t l = msec & 0xff;
  while (T3_COUNT_H < h);
  while (T3_COUNT_L < l);
}

bool timer3_tick_gt(uint16_t msec) {
  msec <<= 4;
  uint8_t h = msec >> 8;
  uint8_t t = T3_COUNT_H;
  if (t < h)
    return false;
  if (t > h)
    return true;
  uint8_t l = msec & 0xff;
  t = T3_COUNT_L;
  if (t > l)
    return true;
  return T3_COUNT_H > h;
}

uint16_t timer3_tick_msec() {
  T3_CTRL &= ~bT3_CNT_EN;  // Stop counting
  uint16_t tick = ((uint16_t)T3_COUNT_H << 4) | (T3_COUNT_L >> 4);
  T3_CTRL |= bT3_CNT_EN;  // Start counting
  return tick;
}

uint16_t timer3_tick_sec() {
  IE_TMR3 = 0;
  uint16_t tick = timer3_tick;
  IE_TMR3 = 1;
  return tick;
}

void delayMicroseconds(uint32_t us) {
  while (us > 255) {
    delayU8us(255);
    us -= 255;
  }
  delayU8us(us & 0xff);
}

void delay(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i)
    delayMicroseconds(1000);
}

void pinMode(uint8_t port, uint8_t bit, uint8_t mode) {
  uint8_t mask = 1 << bit;
  if (mode == INPUT_PULLUP) {
    switch (port) {
      case 0:
        P0_PU |= mask;
        break;
      case 1:
        P1_PU |= mask;
        break;
      case 2:
        P2_PU |= mask;
        break;
      case 3:
        P3_PU |= mask;
        break;
      case 4:
        P4_PU |= mask;
        break;
      default:
        Serial.println("N/A");
        break;
    }
  } else {
    mask = ~mask;
    switch (port) {
      case 0:
        P0_PU &= mask;
        break;
      case 1:
        P1_PU &= mask;
        break;
      case 2:
        P2_PU &= mask;
        break;
      case 3:
        P3_PU &= mask;
        break;
      case 4:
        P4_PU &= mask;
        break;
      default:
        Serial.println("N/A");
        break;
    }
    mask = ~mask;
  }
  if (mode == OUTPUT) {
    switch (port) {
      case 0:
        P0_DIR |= mask;
        break;
      case 1:
        P1_DIR |= mask;
        break;
      case 2:
        P2_DIR |= mask;
        break;
      case 3:
        P3_DIR |= mask;
        break;
      case 4:
        P4_DIR |= mask;
        break;
      default:
        Serial.println("N/A");
        break;
    }
  } else {
    mask = ~mask;
    switch (port) {
      case 0:
        P0_DIR &= mask;
        break;
      case 1:
        P1_DIR &= mask;
        break;
      case 2:
        P2_DIR &= mask;
        break;
      case 3:
        P3_DIR &= mask;
        break;
      case 4:
        P4_DIR &= mask;
        break;
      default:
        Serial.println("N/A");
        break;
    }
  }
}

void digitalWrite(uint8_t port, uint8_t bit, uint8_t value) {
  uint8_t mask = 1 << bit;
  if (value == HIGH) {
    switch (port) {
      case 0:
        P0 |= mask;
        break;
      case 1:
        P1 |= mask;
        break;
      case 2:
        P2 |= mask;
        break;
      case 3:
        P3 |= mask;
        break;
      default:
        Serial.println("N/A");
        break;
    }
  } else {
    mask = ~mask;
    switch (port) {
      case 0:
        P0 &= mask;
        break;
      case 1:
        P1 &= mask;
        break;
      case 2:
        P2 &= mask;
        break;
      case 3:
        P3 &= mask;
        break;
      default:
        Serial.println("N/A");
        break;
    }
  }
}

uint8_t digitalReadPort(uint8_t port) {
  switch (port) {
    case 0:
      return P0;
    case 1:
      return P1;
    case 2:
      return P2;
    case 3:
      return P3;
    case 4:
      return P4_IN;
    default:
      Serial.println("N/A");
      return 0;
  }
}