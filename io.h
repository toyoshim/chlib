// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __io_h__
#define __io_h__

#include <compiler.h>

SFR(P0, 0x80);  // P0 input/output register
SFR(ROM_ADDR_L, 0x84);  // Flash-ROM address register low byte
SFR(ROM_ADDR_H, 0x85);  // Flash-ROM address register high byte
SFR(ROM_STATUS, 0x86);  // Flash-ROM status register
SFR(ROM_CTRL, 0x86);  // Flash-ROM control register
SFR(ROM_DATA_L, 0x8e);  // Flash-ROM data register low byte
SFR(ROM_DATA_H, 0x8f);  // Flash-ROM data register high byte
SFR(PCON, 0x87);  // Power control register
SFR(TMOD, 0x89);  // Timer/counter/0/1 mode register
SFR(TH1, 0x8d);	 // High byte of timer1 count
SFR(P1, 0x90);  // P1 input/output register
SFR(SER1_IER, 0x91);  // UART1 interrupt enable
SFR(SER1_DLM, 0x91);  // UART1 divisor latch MSB byte, only when DLAB=1
SFR(SER1_FCR, 0x92);  // UART1 FIFO control
SFR(SER1_LCR, 0x93);  // UART1 line control
SFR(SER1_MCR, 0x94);  // UART1 modem control
SFR(SER1_LSR, 0x95);  // UART1 line status
SFR(SER1_DIV, 0x97);  // UART1 pre-divisor latch byte, only when DLAB=1
SFR(SBUF, 0x99);  // UART0 data buffer register
SFR(SER1_FIFO, 0x9a);  // UART1 FIFO register
SFR(SER1_DLL, 0x9a);  // UART1 divisor latch LSB byte, only when DLAB=1
SFR(PWM_DATA, 0x9c);  // PWM1 data register
SFR(PWM_CTRL, 0x9d);  // PWM control register
SFR(PWM_CK_SE, 0x9e);  // PWM clock divisor register
SFR(PWM_CYCLE, 0x9f);  // PWM cycle period register
SFR(P2, 0xa0);  // P2 input/output register
SFR(SAFE_MOD, 0xa1);  // Safe mode control register
SFR(T3_SETUP, 0xa3);  // Timer3 setup register
SFR(T3_CK_SE_L, 0xa4);  // Timer3 clock divisor setting low byte
SFR(T3_COUNT_L, 0xa4);  // Timer3 current count low byte
SFR(T3_CK_SE_H, 0xa5);  // Timer3 clock divisor settings high byte
SFR(T3_COUNT_H, 0xa5);  // Timer3 current count high byte
SFR(T3_END_L, 0xa6);  // Timer3 count end value low byte
SFR(T3_END_H, 0xa7);  // Timer3 count end value high byte
SFR(IE, 0xa8);  // Interrupt enable register
SFR(T3_STAT, 0xa9);  // Timer3 status register
SFR(T3_CTRL, 0xaa);  // Timer3 control register
SFR(P3, 0xb0);  // P3 input/output register
SFR(GLOBAL_CFG, 0xb1);  // Global configuration register
SFR(PLL_CFG, 0xb2);  // PLL clock configuration register
SFR(CLOCK_CFG, 0xb3);  // System clock configuration register
SFR(P1_DIR, 0xba);  // P1 direction control register
SFR(P1_PU, 0xbb);  // P1 pull-up enable register
SFR(P2_DIR, 0xbc);  // P2 direction control register
SFR(P2_PU, 0xbd);  // P2 pull-up enable register
SFR(P3_DIR, 0xbe);  // P3 direction control register
SFR(P3_PU, 0xbf);  // P3 pull-up enable register
SFR(P4_OUT, 0xc0);  // P4 output register
SFR(P4_IN, 0xc1);  // P4 input register
SFR(P4_DIR, 0xc2);  // P4 direction control register
SFR(P4_PU, 0xc3);  // P4 pull-up enable register
SFR(P0_DIR, 0xc4);  // P0 direction control register
SFR(P0_PU, 0xc5);  // P0 pull-up enable register
SFR(PORT_CFG, 0xc6);  // Port configuration register
SFR(T2MOD, 0xc9);  // Timer2 mode register
SFR(PIN_FUNC, 0xce);  // Function pins select register
SFR(USB_RX_LEN, 0xd1);  // USB receiving length register
SFR(UEP1_CTRL, 0xd2);  // Endpoint1 control register
SFR(UH_SETUP, 0xd2);  // USB host auxiliary setup register
SFR(UEP1_T_LEN, 0xd3);  // Endpoint1 transmittal length register
SFR(UEP2_CTRL, 0xd4);  // Endpoint2 control register
SFR(UH_RX_CTRL, 0xd4);  // USB host receiver endpoint control register
SFR(UH_EP_PID, 0xd5);  // USB host endpoint and token PID register
SFR(UH_TX_CTRL, 0xd6);  // USB host transmittal endpoint control register
SFR(UH_TX_LEN, 0xd7);  // USB host transmittal length register
SFR(USB_INT_FG, 0xd8);  // USB interrupt flag register
SFR(USB_INT_ST, 0xd9);  // USB interrupt status
SFR(USB_MIS_ST, 0xda);  // USB miscellaneous status
SFR(USB_HUB_ST, 0xdb);  // USB host hub status
SFR(UEP0_CTRL, 0xdc);  // Endpoint0 control register
SFR(UEP0_T_LEN, 0xdd);  // Endpoint0 transmittal length reigster
SFR(USB_INT_EN, 0xe1);  // USB interrrupt enable
SFR(USB_CTRL, 0xe2);  // USB control register
SFR(USB_DEV_AD, 0xe3);  // USB device address register
SFR(UDEV_CTRL, 0xe4);  // USB device port control register
SFR(UHUB0_CTRL, 0xe4);  // USB HUB0 control register
SFR(UHUB1_CTRL, 0xe5);  // USB HUB1 control register
SFR(IE_EX, 0xe8);  // Extend interrupt enable register
SFR(RESET_KEEP, 0xfe);  // Reset-keeping register

SBIT(TR1, 0x88, 6);  // TCON, Timer1 start/stop bit
SBIT(SM1, 0x98, 6);  // SCON, UART0 mode bit1, selection baud rate
SBIT(SM0, 0x98, 7);  // SCON, UART0 mode bit0, selection data bit
SBIT(TI, 0x98, 1);  // SCON, Transmit interrupt flag
SBIT(EA, 0xa8, 7);  // IE, Global interrupt enable control bit
SBIT(UIF_BUS_RST, 0xd8, 0);  // USB_INT_FG, USB bus reset (device)
SBIT(UIF_DETECT, 0xd8, 0);  // USB_INT_FG, checking device connection (host)
SBIT(UIF_TRANSFER, 0xd8, 1);  // USB_INT_FG, USB transfer complete
SBIT(UIF_SUSPEND, 0xd8, 2);  // USB_INT_FG, USB suspend or resume
SBIT(U_TOG_OK, 0xd8, 6);  // USB_INT_FG, Current USB transmit data toggle flag
SBIT(IE_TMR3, 0xe8, 1);  // IE_EX, Timer3 interruprt enable bit
SBIT(IE_USB, 0xe8, 2);  // IE_EX, USB interruprt enable bit

enum {
  SMOD = 0x80,  // PCON, Baud rate selection for UART0 mode 1/2/3
  bT1_M1 = 0x20,  // TMOD, Timer1 mode high bit
  bIER_PIN_MOD1 = 0x20,  // SER1_IER, UART1 pin mode high bit
  bFCR_FIFO_EN = 0x01,  // SER1_FCR, UART1 FIFO enable
  bFCR_R_FIFO_CLR = 0x02,  // SER1_FCR, UART1 receiver FIFO clear
  bFCR_T_FIFO_CLR = 0x04,  // SER1_FCR, UART1 transmitter FIFO clear
  bLCR_WORD_SZ0 = 0x01,  // SER1_LCR, UART1 word bit length low bit
  bLCR_WORD_SZ1 = 0x02,  // SER1_LCR, UART1 word bit length high bit
  bLCR_DLAB = 0x80,  // SER1_LCR, UART1 divisor latch access bit enable
  bMCR_HALF = 0x80,  // SE1_MCR, UART1 enable half-duplex mode
  bLSR_T_FIFO_EMP = 0x20,  // SER1_LSR, UART1 transmitter FIFO empty status
  bLSR_DATA_RDY = 0x01,  // SER1_LSR, UART1 receiver FIFO data ready status
  bPWM_CLR_ALL = 0x02,  // PWM_CTRL, clear FIFO and count of PWM1/2
  bPWM_OUT_EN = 0x08,  // PWM_CTRL, PWM1 output enable
  bT3_EN_CK_SE = 0x01,  // T3_SETUP, enable to access divisor setting register
  bT3_IE_END = 0x80,  // T3_SETUP, enable interrupt for capture mode count
  bT3_IF_END = 0x10,  // T3_STAT, Interrupt flag for count over
  bT3_MOD_CAP = 0x01,  // T3_CTRL, Timer3 mode
  bT3_CLR_ALL = 0x02,  // T3_CTRL, Timer3 force clear FIFO and count
  bT3_CNT_EN = 0x04,  // T3_CTRL, Timer3 count enable
  bDATA_WE = 0x04,  // GLOBAL_CFG, Data-Flash program or erase enable
  bCODE_WE = 0x08,  // GLOBAL_CFG, Flash-ROM program or erase enable
  MASK_SYS_CK_DIV = 0x1f,  // CLOCK_CFG, system clock divisor factor
  bTMR_CLK = 0x80,  // T2MOD, Fastest internal clock mode for timer 0/1/2
  bT1_CLK = 0x20,  // T2MOD, Timer1 internal clock frequency selection
  bUART0_PIN_X = 0x10,  // PIN_FUNC, Pin UART0 mapping enable bit
  bPWM1_PIN_X = 0x80,  // PIN_FUNC, Pin PWM1/PWM2 mapping enable bit
  bUH_R_TOG = 0x80,  // UH_RX_CTRL, Expected data toggle flag
  bUH_R_AUTO_TOG = 0x10,  // UH_RX_CTRL, Enable automatic toggle
  bUH_T_TOG = 0x40,  // UH_TX_CTRL, Prepared data toggle flag
  bUH_T_AUTO_TOG = 0x10,  // UH_TX_CTRL, Enable automatic toggle
  MASK_UIS_ENDP = 0x0f,  // USB_INT_ST, mask for endpoint number (device)
  MASK_UIS_HRES = 0x0f,  // USB_INT_ST, mask for current token PID (host)
  MASK_UIS_TOKEN = 0x30,  // USB_INT_ST, mask for bUIS_TOKEN1,0
  UIS_TOKEN_OUT = 0x00,  // USB_INT_ST, OUT packet token
  UIS_TOKEN_SOF = 0x10,  // USB_INT_ST, SOF packet token
  UIS_TOKEN_IN = 0x20,  // USB_INT_ST, IN packet token
  UIS_TOKEN_SETUP = 0x30,  // USB_INT_ST, SETUP packet token
  bUHS_BUS_RESET = 0x02,  // USB_HUB_ST, Control USB hub bus reset
  bUHS_DM_LEVEL = 0x04,  // USB_HUB_ST, DM level on attaching
  bUHS_H0_ATTACH = 0x08,  // USB_HUB_ST, USB HUB0 attached status
  bUHS_HM_LEVEL = 0x40,  // USB_HUB_ST, HM level on attaching
  bUHS_H1_ATTACH = 0x80,  // USB_HUB_ST, USB HUB1 attached status
  bUH_PORT_EN = 0x01,  // UHUBn_CTRL, Enable USB hub port
  bUH_LOW_SPEED = 0x04,  // UHUBn_CTRL, Enable USB hub low speed
  bUEP_R_TOG = 0x80,  // UEPx_CTRL, Expected data toggle flag of USB EPn RX
  bUEP_T_TOG = 0x40,  // UEPx_CTRL, Expected data toggle flag of USB EPn TX
  MASK_UEP_T_RES = 0x03,  // UEPx_CTRL, mask for UEP_T_RES_*
  UEP_T_RES_ACK = 0x00,  // UEPx_CTRL, Handshake ack response for EPn TX
  UEP_T_RES_NAK = 0x02,  // UEPx_CTRL, Handshake nak response for EPn TX
  UEP_T_RES_STALL = 0x03,  // UEPx_CTRL, Handshake stall response for EPn TX
  UEP_R_RES_ACK = 0x00,  // UEPx_CTRL, Handshake ack response for EPn RX
  UEP_R_RES_NAK = 0x80,  // UEPx_CTRL, Handshake nak response for EPn RX
  UEP_R_RES_STALL = 0xc0,  // UEPx_CTRL, Handshake stall response for EPn RX
  bUEP_AUTO_TOG = 0x10,  // UEPx_CTRL, automatic toggle
  bUH_SOF_EN = 0x40,  // UH_SETUP, USB host automatic SOF enable
  bUIE_BUS_RST = 0x01,  // USB_INT_EN, USB bus reset event (device)
  bUIE_DETECT = 0x01,  // USB_INT_EN, USB device detected event (host)
  bUIE_TRANSFER = 0x02,  // USB_INT_EN, USB transfer complete interrupt flag
  bUIE_SUSPEND = 0x04,  // USB_INT_EN, USB suspend or resume interrupt flag
  bUC_DMA_EN = 0x01,  // USB_CTRL, Enable DMA and interrupt
  bUC_INT_BUSY = 0x08,  // USB_CTRL, Automatic responding busy
  bUC_DEV_PU_EN = 0x20,  // USB_CTRL, Enable USB device function
  bUC_LOW_SPEED = 0x40,  // USB_CTRL, USB bus speed selection
  bUC_HOST_MODE = 0x80,  // USB_CTRL, USB mode selection
  bUMS_SUSPEND = 0x04,  // USB_MIS_ST, USB suspend status
  bUD_PORT_EN = 0x01,  // UDEV_CTRL, Enable USB physical port I/O
  bUD_DM_PD_DIS = 0x10,  // UDEV_CTRL, Disable USB DM pull-down register
  bUD_DP_PD_DIS = 0x20,  // UDEV_CTRL, Disable USB DP pull-down register
  bUEP1_TX_EN = 0x40,  // UEP4_1_MOD, Enable USB endpoint 1 receiving
  bUH_EP_RX_EN = 0x08,  // UH_EP_MOD, Enable USB host endpoint receiving
  bUH_EP_TX_EN = 0x40,  // UH_EP_MOD, Enable USB host endpoint transmittal
};

__at (0x2446) uint8_t volatile UEP4_1_MOD;
__at (0x2447) uint8_t volatile UEP2_3_MOD;
__at (0x2447) uint8_t volatile UH_EP_MOD;
__at (0x2448) uint8_t volatile UEP0_DMA_H;
__at (0x2449) uint8_t volatile UEP0_DMA_L;
__at (0x244a) uint8_t volatile UEP1_DMA_H;
__at (0x244b) uint8_t volatile UEP1_DMA_L;
__at (0x244c) uint8_t volatile UH_RX_DMA_H;
__at (0x244d) uint8_t volatile UH_RX_DMA_L;
__at (0x244e) uint8_t volatile UH_TX_DMA_H;
__at (0x244f) uint8_t volatile UH_TX_DMA_L;

#endif  // __io_h__