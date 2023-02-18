/*
 * tusb_config.h
 *
 * This file is auto-generated. Please do not edit!
 *
 */

#pragma once

#define CFG_TUD_ENABLED       1
// RHPort number used for device can be defined by board.mk, default to port 0
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif

// RHPort max operational speed can defined by board.mk
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

// CFG_TUSB_DEBUG is defined by compiler in DEBUG build
// #define CFG_TUSB_DEBUG           0

/* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
 * Tinyusb use follows macros to declare transferring memory so that they can be put
 * into those specific section.
 * e.g
 * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
 * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
 */
#define CFG_TUSB_MEM_SECTION

#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_ENDPOINT0_SIZE 64

//------------- CLASS -------------//

${class_counts}

// HID buffer size must be sufficient to hold ID (if any) + Data
#define CFG_TUD_HID_EP_BUFSIZE ${hid_ep_bufsize}

// CDC FIFO size of TX and RX
#define CFG_TUD_CDC_RX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)
#define CFG_TUD_CDC_TX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)

// Vendor FIFO size of TX and RX
// If not configured vendor endpoints will not be buffered
#define CFG_TUD_VENDOR_RX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)
#define CFG_TUD_VENDOR_TX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)

// MSC Buffer size of Device Mass storage
#define CFG_TUD_MSC_EP_BUFSIZE 512

// MIDI FIFO size of TX and RX
#define CFG_TUD_MIDI_RX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)
#define CFG_TUD_MIDI_TX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)

//--------------------------------------------------------------------
// HOST CONFIGURATION
//--------------------------------------------------------------------

// Size of buffer to hold descriptors and other data used for enumeration
#define CFG_TUH_ENUMERATION_BUFSIZE 256

// max device support (excluding hub device)
#define CFG_TUH_DEVICE_MAX (CFG_TUH_HUB ? 4 : 1) // hub typically has 4 ports

//------------- HID -------------//
#define CFG_TUH_HID_EPIN_BUFSIZE 64
#define CFG_TUH_HID_EPOUT_BUFSIZE 64

//------------- CDC -------------//

// Set Line Control state on enumeration/mounted:
// DTR ( bit 0), RTS (bit 1)
#define CFG_TUH_CDC_LINE_CONTROL_ON_ENUM 0x03

// Set Line Coding on enumeration/mounted, value for cdc_line_coding_t
// bit rate = 115200, 1 stop bit, no parity, 8 bit data width
#define CFG_TUH_CDC_LINE_CODING_ON_ENUM                                                                                \
	{                                                                                                                  \
		115200, CDC_LINE_CONDING_STOP_BITS_1, CDC_LINE_CODING_PARITY_NONE, 8                                           \
	}
