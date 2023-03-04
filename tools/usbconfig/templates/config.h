/*
 * tusb_config.h
 *
 * This file is auto-generated. Please do not edit!
 *
 */

#pragma once

// Use C++ support classes
#define ENABLE_USB_CLASSES 1

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif

#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_ENABLED ${device_enabled}

// Device interface classes
${device_classes}

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

// DFU buffer size, it has to be set to the buffer size used in TUD_DFU_DESCRIPTOR
#define CFG_TUD_DFU_XFER_BUFSIZE  (TUD_OPT_HIGH_SPEED ? 512 : 64)

//--------------------------------------------------------------------
// HOST CONFIGURATION
//--------------------------------------------------------------------

#ifndef BOARD_TUH_RHPORT
#define BOARD_TUH_RHPORT      0
#endif

#define CFG_TUH_ENABLED ${host_enabled}
#define CFG_TUH_HUB 1
#define CFG_TUH_API_EDPT_XFER 1

// Host interface classes
${host_classes}

// max device support (excluding hub device)
#define CFG_TUH_DEVICE_MAX (CFG_TUH_HUB ? 4 : 1) // hub typically has 4 ports

#define CFG_TUH_CDC_RX_BUFSIZE (TUH_OPT_HIGH_SPEED ? TUSB_EPSIZE_BULK_HS : TUSB_EPSIZE_BULK_FS)
#define CFG_TUH_CDC_TX_BUFSIZE (TUH_OPT_HIGH_SPEED ? TUSB_EPSIZE_BULK_HS : TUSB_EPSIZE_BULK_FS)
