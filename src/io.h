// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __io_h__
#define __io_h__

#include <compiler.h>

SFR(P0, 0x80);          // P0 input/output register
SFR(ROM_ADDR_L, 0x84);  // Flash-ROM address register low byte
SFR(ROM_ADDR_H, 0x85);  // Flash-ROM address register high byte
SFR(ROM_STATUS, 0x86);  // Flash-ROM status register
SFR(ROM_CTRL, 0x86);    // Flash-ROM control register
SFR(TL0, 0x8a);         // Timer0 count register low byte
SFR(TH0, 0x8c);         // Timer0 count register high byte
SFR(ROM_DATA_L, 0x8e);  // Flash-ROM data register low byte
SFR(ROM_DATA_H, 0x8f);  // Flash-ROM data register high byte
SFR(PCON, 0x87);        // Power control register
SFR(TMOD, 0x89);        // Timer/counter/0/1 mode register
SFR(TH1, 0x8d);         // High byte of timer1 count
SFR(P1, 0x90);          // P1 input/output register
SFR(SER1_IER, 0x91);    // UART1 interrupt enable
SFR(SER1_DLM, 0x91);    // UART1 divisor latch MSB byte, only when DLAB=1
SFR(SER1_IIR, 0x92);    // UART1 interrupt identification
SFR(SER1_FCR, 0x92);    // UART1 FIFO control
SFR(SER1_LCR, 0x93);    // UART1 line control
SFR(SER1_MCR, 0x94);    // UART1 modem control
SFR(SER1_LSR, 0x95);    // UART1 line status
SFR(SER1_DIV, 0x97);    // UART1 pre-divisor latch byte, only when DLAB=1
SFR(SER1_ADDR, 0x97);   // UART1 bus address preset register
SFR(SBUF, 0x99);        // UART0 data buffer register
SFR(SER1_FIFO, 0x9a);   // UART1 FIFO register
SFR(SER1_DLL, 0x9a);    // UART1 divisor latch LSB byte, only when DLAB=1
SFR(PWM_DATA, 0x9c);    // PWM1 data register
SFR(PWM_CTRL, 0x9d);    // PWM control register
SFR(PWM_CK_SE, 0x9e);   // PWM clock divisor register
SFR(PWM_CYCLE, 0x9f);   // PWM cycle period register
SFR(P2, 0xa0);          // P2 input/output register
SFR(SAFE_MOD, 0xa1);    // Safe mode control register
SFR(T3_SETUP, 0xa3);    // Timer3 setup register
SFR(T3_CK_SE_L, 0xa4);  // Timer3 clock divisor setting low byte
SFR(T3_COUNT_L, 0xa4);  // Timer3 current count low byte
SFR(T3_CK_SE_H, 0xa5);  // Timer3 clock divisor settings high byte
SFR(T3_COUNT_H, 0xa5);  // Timer3 current count high byte
SFR(T3_END_L, 0xa6);    // Timer3 count end value low byte
SFR(T3_END_H, 0xa7);    // Timer3 count end value high byte
SFR(IE, 0xa8);          // Interrupt enable register
SFR(T3_STAT, 0xa9);     // Timer3 status register
SFR(T3_CTRL, 0xaa);     // Timer3 control register
SFR(P3, 0xb0);          // P3 input/output register
SFR(GLOBAL_CFG, 0xb1);  // Global configuration register
SFR(PLL_CFG, 0xb2);     // PLL clock configuration register
SFR(CLOCK_CFG, 0xb3);   // System clock configuration register
SFR(P1_IE, 0xb9);       // P1 input enable register
SFR(P1_DIR, 0xba);      // P1 direction control register
SFR(P1_PU, 0xbb);       // P1 pull-up enable register
SFR(P2_DIR, 0xbc);      // P2 direction control register
SFR(P2_PU, 0xbd);       // P2 pull-up enable register
SFR(P3_DIR, 0xbe);      // P3 direction control register
SFR(P3_PU, 0xbf);       // P3 pull-up enable register
SFR(P4_OUT, 0xc0);      // P4 output register
SFR(P4_IN, 0xc1);       // P4 input register
SFR(P4_DIR, 0xc2);      // P4 direction control register
SFR(P4_PU, 0xc3);       // P4 pull-up enable register
SFR(P0_DIR, 0xc4);      // P0 direction control register
SFR(P0_PU, 0xc5);       // P0 pull-up enable register
SFR(PORT_CFG, 0xc6);    // Port configuration register
SFR(P5_IN, 0xc7);       // P5 input register
SFR(T2MOD, 0xc9);       // Timer2 mode register
SFR(PIN_FUNC, 0xce);    // Function pins select register
SFR(USB_RX_LEN, 0xd1);  // USB receiving length register
SFR(UEP1_CTRL, 0xd2);   // Endpoint1 control register
SFR(UH_SETUP, 0xd2);    // USB host auxiliary setup register
SFR(UEP1_T_LEN, 0xd3);  // Endpoint1 transmittal length register
SFR(UEP2_CTRL, 0xd4);   // Endpoint2 control register
SFR(UH_RX_CTRL, 0xd4);  // USB host receiver endpoint control register
SFR(UEP2_T_LEN, 0xd5);  // Endpoint2 transmittal length register
SFR(UH_EP_PID, 0xd5);   // USB host endpoint and token PID register
SFR(UEP3_CTRL, 0xd6);   // Endpoint3 control register
SFR(UH_TX_CTRL, 0xd6);  // USB host transmittal endpoint control register
SFR(UEP3_T_LEN, 0xd7);  // Endpoint3 transmittal length register
SFR(UH_TX_LEN, 0xd7);   // USB host transmittal length register
SFR(USB_INT_FG, 0xd8);  // USB interrupt flag register
SFR(USB_INT_ST, 0xd9);  // USB interrupt status
SFR(USB_MIS_ST, 0xda);  // USB miscellaneous status
SFR(USB_HUB_ST, 0xdb);  // USB host hub status
SFR(UEP0_CTRL, 0xdc);   // Endpoint0 control register
SFR(UEP0_T_LEN, 0xdd);  // Endpoint0 transmittal length reigster
SFR(USB_INT_EN, 0xe1);  // USB interrrupt enable
SFR(USB_CTRL, 0xe2);    // USB control register
SFR(USB_DEV_AD, 0xe3);  // USB device address register
SFR(UDEV_CTRL, 0xe4);   // USB device port control register
SFR(UHUB0_CTRL, 0xe4);  // USB HUB0 control register
SFR(UHUB1_CTRL, 0xe5);  // USB HUB1 control register
SFR(IE_EX, 0xe8);       // Extend interrupt enable register
SFR(ADC_CK_SE, 0xef);   // ADC clock divisor setting
SFR(ADC_STAT, 0xf1);    // ADC status
SFR(ADC_CTRL, 0xf2);    // ADC control
SFR(ADC_CHANN, 0xf3);   // ADC channel selection
SFR(ADC_FIFO_L, 0xf4);  // ADC FIFO low byte
SFR(ADC_FIFO_H, 0xf5);  // ADC FIFO high byte
SFR(ADC_SETUP, 0xf6);   // ADC setup
SFR(ADC_EX_SW, 0xf7);   // ADC extend switch control
SFR(RESET_KEEP, 0xfe);  // Reset-keeping register

SBIT(P0_0, 0x80, 0);          // P0, 0
SBIT(P0_1, 0x80, 1);          // P0, 1
SBIT(P0_2, 0x80, 2);          // P0, 2
SBIT(P0_3, 0x80, 3);          // P0, 3
SBIT(P0_4, 0x80, 4);          // P0, 4
SBIT(P0_5, 0x80, 5);          // P0, 5
SBIT(P0_6, 0x80, 6);          // P0, 6
SBIT(P0_7, 0x80, 7);          // P0, 7
SBIT(TR0, 0x88, 4);           // TCON, Timer0 start/stop bit
SBIT(TR1, 0x88, 6);           // TCON, Timer1 start/stop bit
SBIT(P1_0, 0x90, 0);          // P1, 0
SBIT(P1_1, 0x90, 1);          // P1, 1
SBIT(P1_2, 0x90, 2);          // P1, 2
SBIT(P1_3, 0x90, 3);          // P1, 3
SBIT(P1_4, 0x90, 4);          // P1, 4
SBIT(P1_5, 0x90, 5);          // P1, 5
SBIT(P1_6, 0x90, 6);          // P1, 6
SBIT(P1_7, 0x90, 7);          // P1, 7
SBIT(SM1, 0x98, 6);           // SCON, UART0 mode bit1, selection baud rate
SBIT(SM0, 0x98, 7);           // SCON, UART0 mode bit0, selection data bit
SBIT(TI, 0x98, 1);            // SCON, Transmit interrupt flag
SBIT(P2_0, 0xa0, 0);          // P2, 0
SBIT(P2_1, 0xa0, 1);          // P2, 1
SBIT(P2_2, 0xa0, 2);          // P2, 2
SBIT(P2_3, 0xa0, 3);          // P2, 3
SBIT(P2_4, 0xa0, 4);          // P2, 4
SBIT(P2_5, 0xa0, 5);          // P2, 5
SBIT(P2_6, 0xa0, 6);          // P2, 6
SBIT(P2_7, 0xa0, 7);          // P2, 7
SBIT(ET0, 0xa8, 1);           // IE, Timer0 interrupt enable bit
SBIT(EA, 0xa8, 7);            // IE, Global interrupt enable control bit
SBIT(P4_0, 0xc0, 0);          // P4, 0
SBIT(P4_1, 0xc0, 1);          // P4, 1
SBIT(P4_2, 0xc0, 2);          // P4, 2
SBIT(P4_3, 0xc0, 3);          // P4, 3
SBIT(P4_4, 0xc0, 4);          // P4, 4
SBIT(P4_5, 0xc0, 5);          // P4, 5
SBIT(P4_6, 0xc0, 6);          // P4, 6
SBIT(P4_7, 0xc0, 7);          // P4, 7
SBIT(UIF_BUS_RST, 0xd8, 0);   // USB_INT_FG, USB bus reset (device)
SBIT(UIF_DETECT, 0xd8, 0);    // USB_INT_FG, checking device connection (host)
SBIT(UIF_TRANSFER, 0xd8, 1);  // USB_INT_FG, USB transfer complete
SBIT(UIF_SUSPEND, 0xd8, 2);   // USB_INT_FG, USB suspend or resume
SBIT(U_TOG_OK, 0xd8, 6);      // USB_INT_FG, Current USB transmit data toggle
SBIT(IE_TMR3, 0xe8, 1);       // IE_EX, Timer3 interruprt enable bit
SBIT(IE_USB, 0xe8, 2);        // IE_EX, USB interruprt enable bit
SBIT(IE_UART1, 0xe8, 4);      // IE_EX, UART1 interruprt enable bit

enum {
  SMOD = 0x80,             // PCON, Baud rate selection for UART0 mode 1/2/3
  bT0_M0 = 0x01,           // TMOD, Timer0 mode low bit
  bT0_M1 = 0x02,           // TMOD, Timer0 mode high bit
  bT1_M1 = 0x20,           // TMOD, Timer1 mode high bit
  bIER_RESET = 0x80,       // SER1_IER, UART1 software reset
  bIER_PIN_MOD1 = 0x20,    // SER1_IER, UART1 pin mode high bit
  bIER_PIN_MOD0 = 0x10,    // SER1_IER, UART1 pin mode low bit
  bIER_RECV_RDY = 0x01,    // SER1_IER, UART1 receiver data ready
  bFCR_FIFO_EN = 0x01,     // SER1_FCR, UART1 FIFO enable
  bFCR_R_FIFO_CLR = 0x02,  // SER1_FCR, UART1 receiver FIFO clear
  bFCR_T_FIFO_CLR = 0x04,  // SER1_FCR, UART1 transmitter FIFO clear
  bLCR_WORD_SZ0 = 0x01,    // SER1_LCR, UART1 word bit length low bit
  bLCR_WORD_SZ1 = 0x02,    // SER1_LCR, UART1 word bit length high bit
  bLCR_DLAB = 0x80,        // SER1_LCR, UART1 divisor latch access bit enable
  bMCR_OUT2 = 0x08,        // SER1_MCR, UART1 enable interrupt requuest output
  bMCR_HALF = 0x80,        // SER1_MCR, UART1 enable half-duplex mode
  bLSR_T_ALL_EMP = 0x40,   // SER1_LSR, UART1 transmitter all empty status
  bLSR_T_FIFO_EMP = 0x20,  // SER1_LSR, UART1 transmitter FIFO empty status
  bLSR_OVER_ERR = 0x02,    // SER1_LSR, UART1 receiver overflow error
  bLSR_DATA_RDY = 0x01,    // SER1_LSR, UART1 receiver FIFO data ready status
  bPWM_CLR_ALL = 0x02,     // PWM_CTRL, clear FIFO and count of PWM1/2
  bPWM_OUT_EN = 0x08,      // PWM_CTRL, PWM1 output enable
  bT3_EN_CK_SE = 0x01,     // T3_SETUP, enable to access divisor setting
  bT3_IE_END = 0x80,       // T3_SETUP, enable interrupt for capture mode count
  bT3_IF_END = 0x10,       // T3_STAT, Interrupt flag for count over
  bT3_MOD_CAP = 0x01,      // T3_CTRL, Timer3 mode
  bT3_CLR_ALL = 0x02,      // T3_CTRL, Timer3 force clear FIFO and count
  bT3_CNT_EN = 0x04,       // T3_CTRL, Timer3 count enable
  bDATA_WE = 0x04,         // GLOBAL_CFG, Data-Flash program or erase enable
  bCODE_WE = 0x08,         // GLOBAL_CFG, Flash-ROM program or erase enable
  MASK_SYS_CK_DIV = 0x1f,  // CLOCK_CFG, system clock divisor factor
  bT0_CLK = 0x10,          // T2MOD, Timer0 internal clock frequency selection
  bT1_CLK = 0x20,          // T2MOD, Timer1 internal clock frequency selection
  bTMR_CLK = 0x80,         // T2MOD, Fastest internal clock mode for timer 0/1/2
  bP4_DRV = 0x04,          // P5_IN, P4 output ability select
  bUART0_PIN_X = 0x10,     // PIN_FUNC, Pin UART0 mapping enable bit
  bPWM1_PIN_X = 0x80,      // PIN_FUNC, Pin PWM1/PWM2 mapping enable bit
  bUH_R_TOG = 0x80,        // UH_RX_CTRL, Expected data toggle flag
  bUH_R_AUTO_TOG = 0x10,   // UH_RX_CTRL, Enable automatic toggle
  bUH_T_TOG = 0x40,        // UH_TX_CTRL, Prepared data toggle flag
  bUH_T_AUTO_TOG = 0x10,   // UH_TX_CTRL, Enable automatic toggle
  MASK_UIS_ENDP = 0x0f,    // USB_INT_ST, mask for endpoint number (device)
  MASK_UIS_HRES = 0x0f,    // USB_INT_ST, mask for current token PID (host)
  MASK_UIS_TOKEN = 0x30,   // USB_INT_ST, mask for bUIS_TOKEN1,0
  UIS_TOKEN_OUT = 0x00,    // USB_INT_ST, OUT packet token
  UIS_TOKEN_SOF = 0x10,    // USB_INT_ST, SOF packet token
  UIS_TOKEN_IN = 0x20,     // USB_INT_ST, IN packet token
  UIS_TOKEN_SETUP = 0x30,  // USB_INT_ST, SETUP packet token
  bUHS_BUS_RESET = 0x02,   // USB_HUB_ST, Control USB hub bus reset
  bUHS_DM_LEVEL = 0x04,    // USB_HUB_ST, DM level on attaching
  bUHS_H0_ATTACH = 0x08,   // USB_HUB_ST, USB HUB0 attached status
  bUHS_HM_LEVEL = 0x40,    // USB_HUB_ST, HM level on attaching
  bUHS_H1_ATTACH = 0x80,   // USB_HUB_ST, USB HUB1 attached status
  bUH_PORT_EN = 0x01,      // UHUBn_CTRL, Enable USB hub port
  bUH_LOW_SPEED = 0x04,    // UHUBn_CTRL, Enable USB hub low speed
  bUH_DISABLE = 0x80,      // UHUBn_CTRL, Disable USB hub pin (only for UHUB1)
  bUEP_R_TOG = 0x80,       // UEPx_CTRL, Expected data toggle flag of USB EPn RX
  bUEP_T_TOG = 0x40,       // UEPx_CTRL, Expected data toggle flag of USB EPn TX
  MASK_UEP_T_RES = 0x03,   // UEPx_CTRL, mask for UEP_T_RES_*
  UEP_T_RES_ACK = 0x00,    // UEPx_CTRL, Handshake ack response for EPn TX
  UEP_T_RES_NAK = 0x02,    // UEPx_CTRL, Handshake nak response for EPn TX
  UEP_T_RES_STALL = 0x03,  // UEPx_CTRL, Handshake stall response for EPn TX
  UEP_R_RES_ACK = 0x00,    // UEPx_CTRL, Handshake ack response for EPn RX
  UEP_R_RES_NAK = 0x80,    // UEPx_CTRL, Handshake nak response for EPn RX
  UEP_R_RES_STALL = 0xc0,  // UEPx_CTRL, Handshake stall response for EPn RX
  bUEP_AUTO_TOG = 0x10,    // UEPx_CTRL, automatic toggle
  bUH_SOF_EN = 0x40,       // UH_SETUP, USB host automatic SOF enable
  bUIE_BUS_RST = 0x01,     // USB_INT_EN, USB bus reset event (device)
  bUIE_DETECT = 0x01,      // USB_INT_EN, USB device detected event (host)
  bUIE_TRANSFER = 0x02,    // USB_INT_EN, USB transfer complete interrupt flag
  bUIE_SUSPEND = 0x04,     // USB_INT_EN, USB suspend or resume interrupt flag
  bUC_DMA_EN = 0x01,       // USB_CTRL, Enable DMA and interrupt
  bUC_INT_BUSY = 0x08,     // USB_CTRL, Automatic responding busy
  bUC_DEV_PU_EN = 0x20,    // USB_CTRL, Enable USB device function
  bUC_LOW_SPEED = 0x40,    // USB_CTRL, USB bus speed selection
  bUC_HOST_MODE = 0x80,    // USB_CTRL, USB mode selection
  bUMS_SUSPEND = 0x04,     // USB_MIS_ST, USB suspend status
  bUD_PORT_EN = 0x01,      // UDEV_CTRL, Enable USB physical port I/O
  bUD_DM_PD_DIS = 0x10,    // UDEV_CTRL, Disable USB DM pull-down register
  bUD_DP_PD_DIS = 0x20,    // UDEV_CTRL, Disable USB DP pull-down register
  bUEP1_TX_EN = 0x40,      // UEP4_1_MOD, Enable USB endpoint 1 transmittal
  bUEP2_TX_EN = 0x04,      // UEP2_3_MOD, Enable USB endpoint 2 transmittal
  bUEP3_TX_EN = 0x40,      // UEP2_3_MOD, Enable USB endpoint 3 transmittal
  bUH_EP_RX_EN = 0x08,     // UH_EP_MOD, Enable USB host endpoint receiving
  bUH_EP_TX_EN = 0x40,     // UH_EP_MOD, Enable USB host endpoint transmittal
  bADC_IF_ACT = 0x10,      // ADC_STAT, ADC finished flag
  bADC_SAMPLE = 0x80,      // ADC_CTRL, Sampling pulse control
  bADC_POWER_EN = 0x04,    // ADC_SETUP, ADC power control
  bADC_RESOLUTION = 0x04,  // ADC_EX_SW, ADC resolution
};

__at(0x2446) uint8_t volatile UEP4_1_MOD;
__at(0x2447) uint8_t volatile UEP2_3_MOD;
__at(0x2447) uint8_t volatile UH_EP_MOD;
__at(0x2448) uint8_t volatile UEP0_DMA_H;
__at(0x2449) uint8_t volatile UEP0_DMA_L;
__at(0x244a) uint8_t volatile UEP1_DMA_H;
__at(0x244b) uint8_t volatile UEP1_DMA_L;
__at(0x244c) uint8_t volatile UEP2_DMA_H;
__at(0x244d) uint8_t volatile UEP2_DMA_L;
__at(0x244e) uint8_t volatile UEP3_DMA_H;
__at(0x244f) uint8_t volatile UEP3_DMA_L;
__at(0x244c) uint8_t volatile UH_RX_DMA_H;
__at(0x244d) uint8_t volatile UH_RX_DMA_L;
__at(0x244e) uint8_t volatile UH_TX_DMA_H;
__at(0x244f) uint8_t volatile UH_TX_DMA_L;

#endif  // __io_h__