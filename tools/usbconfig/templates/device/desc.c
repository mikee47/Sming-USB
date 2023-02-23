/*
 * usb_descriptors.c
 *
 * This file is auto-generated. Please do not edit!
 *
 */

#include <tusb.h>
#include "usb_descriptors.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

#define USB_BCD   0x0200

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
static const tusb_desc_device_t desc_${device_tag} =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,
    .bDeviceClass       = ${class_id},
    .bDeviceSubClass    = ${subclass_id},
    .bDeviceProtocol    = ${protocol_id},
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = ${vendor_id},
    .idProduct          = USB_PID,
    .bcdDevice          = ${version_bcd},

    .iManufacturer      = ${manufacturer_idx}, // "${manufacturer}"
    .iProduct           = ${product_idx}, // "${product}"
    .iSerialNumber      = ${serial_idx}, // "${serial}"

    .bNumConfigurations = ${config_count},
};

const tusb_desc_device_t* tud_get_device_descriptor(void)
{
  return &desc_${device_tag};
}
