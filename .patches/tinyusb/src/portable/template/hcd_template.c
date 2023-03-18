/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2021 Ha Thach (tinyusb.org) for Double Buffered
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#if CFG_TUSB_MCU == OPT_MCU_NONE

#include "host/hcd.h"

//--------------------------------------------------------------------+
// HCD API
//--------------------------------------------------------------------+
bool hcd_init(uint8_t rhport)
{
  (void) rhport;
  return false;
}

void hcd_port_reset(uint8_t rhport)
{
  (void) rhport;
}

void hcd_port_reset_end(uint8_t rhport)
{
  (void) rhport;
}

bool hcd_port_connect_status(uint8_t rhport)
{
  (void) rhport;
  return false;
}

tusb_speed_t hcd_port_speed_get(uint8_t rhport)
{
  (void) rhport;
    return TUSB_SPEED_FULL;
}

// Close all opened endpoint belong to this device
void hcd_device_close(uint8_t rhport, uint8_t dev_addr)
{
  (void)rhport;
  (void) dev_addr;
}

uint32_t hcd_frame_number(uint8_t rhport)
{
  (void) rhport;
  return 0;
}

void hcd_int_enable(uint8_t rhport)
{
  (void) rhport;
}

void hcd_int_disable(uint8_t rhport)
{
  (void) rhport;
}

//--------------------------------------------------------------------+
// Endpoint API
//--------------------------------------------------------------------+

bool hcd_edpt_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_endpoint_t const * ep_desc)
{
  (void) rhport;
  (void) dev_addr;
  (void) ep_desc;
  return false;
}

bool hcd_edpt_xfer(uint8_t rhport, uint8_t dev_addr, uint8_t ep_addr, uint8_t * buffer, uint16_t buflen)
{
  (void) rhport;
  (void) dev_addr;
  (void) ep_addr;
  (void) buffer;
  (void) buflen;
  return false;
}

bool hcd_setup_send(uint8_t rhport, uint8_t dev_addr, uint8_t const setup_packet[8])
{
  (void) rhport;
  (void) dev_addr;
  (void) setup_packet;
  return false;
}

bool hcd_edpt_clear_stall(uint8_t dev_addr, uint8_t ep_addr)
{
  (void) dev_addr;
  (void) ep_addr;
  return false;
}

#endif
