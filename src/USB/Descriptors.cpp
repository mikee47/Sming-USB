/****
 * Descriptors.cpp
 *
 * Copyright 2023 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the Sming USB Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

#include "USB.h"
#include <Platform/System.h>
#include <Data/HexString.h>

namespace
{
USB::GetDeviceDescriptor deviceDescriptorCallback;
USB::GetDescriptorString descriptorStringCallback;

#define DESC_TYPE_MAP(XX)                                                                                              \
	XX(DEVICE, 0x01)                                                                                                   \
	XX(CONFIGURATION, 0x02)                                                                                            \
	XX(STRING, 0x03)                                                                                                   \
	XX(INTERFACE, 0x04)                                                                                                \
	XX(ENDPOINT, 0x05)                                                                                                 \
	XX(DEVICE_QUALIFIER, 0x06)                                                                                         \
	XX(OTHER_SPEED_CONFIG, 0x07)                                                                                       \
	XX(INTERFACE_POWER, 0x08)                                                                                          \
	XX(OTG, 0x09)                                                                                                      \
	XX(DEBUG, 0x0A)                                                                                                    \
	XX(INTERFACE_ASSOCIATION, 0x0B)                                                                                    \
	XX(BOS, 0x0F)                                                                                                      \
	XX(DEVICE_CAPABILITY, 0x10)                                                                                        \
	XX(CS_DEVICE, 0x21)                                                                                                \
	XX(CS_CONFIGURATION, 0x22)                                                                                         \
	XX(CS_STRING, 0x23)                                                                                                \
	XX(CS_INTERFACE, 0x24)                                                                                             \
	XX(CS_ENDPOINT, 0x25)                                                                                              \
	XX(SUPERSPEED_ENDPOINT_COMPANION, 0x30)                                                                            \
	XX(SUPERSPEED_ISO_ENDPOINT_COMPANION, 0x31)

const char* getDescTypeName(uint8_t type)
{
	switch(type) {
#define XX(name, value)                                                                                                \
	case value:                                                                                                        \
		return #name;
		DESC_TYPE_MAP(XX)
#undef XX
	}
	switch(USB::Descriptor::Type{type}.type) {
	case TUSB_REQ_TYPE_STANDARD:
		return "STANDARD";
	case TUSB_REQ_TYPE_CLASS:
		return "CLASS";
	case TUSB_REQ_TYPE_VENDOR:
		return "VENDOR";
	case TUSB_REQ_TYPE_INVALID:
		return "RESERVED";
	}
}

const char* getXferTypeName(uint8_t type)
{
	switch(type) {
	case TUSB_XFER_CONTROL:
		return "CTRL";
	case TUSB_XFER_ISOCHRONOUS:
		return "ISO";
	case TUSB_XFER_BULK:
		return "BULK";
	case TUSB_XFER_INTERRUPT:
		return "INT";
	default:
		return "?";
	}
}

} // namespace

#if CFG_TUD_ENABLED

extern "C" {
const tusb_desc_device_t* tud_get_device_descriptor(void);
const uint16_t* tud_get_descriptor_string(uint8_t index);
}

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
const uint8_t* tud_descriptor_device_cb(void)
{
	auto desc = tud_get_device_descriptor();
	if(deviceDescriptorCallback) {
		desc = deviceDescriptorCallback(*desc);
	}
	return reinterpret_cast<const uint8_t*>(desc);
}

const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	(void)langid;

	if(descriptorStringCallback) {
		auto desc = descriptorStringCallback(index);
		if(desc) {
			return reinterpret_cast<const uint16_t*>(desc);
		}
	}

	return tud_get_descriptor_string(index);
}

#endif

namespace USB
{
void onGetDeviceDescriptor(GetDeviceDescriptor callback)
{
	deviceDescriptorCallback = callback;
}

void onGetDescriptorSting(GetDescriptorString callback)
{
	descriptorStringCallback = callback;
}

size_t Descriptor::printTo(Print& p) const
{
	size_t n{0};
	n += p.print(getDescTypeName(type));
	n += p.print(": ");
	n += p.print(makeHexString(this, length, ' '));
	switch(type) {
	case TUSB_DESC_INTERFACE: {
		auto itf = as<tusb_desc_interface_t>();
		n += p.println();
		n += p.print("  #");
		n += p.print(itf->bInterfaceNumber);
		n += p.print(", class ");
		n += p.print(itf->bInterfaceClass);
		n += p.print(", subclass ");
		n += p.print(itf->bInterfaceSubClass);
		n += p.print(", protocol ");
		n += p.print(itf->bInterfaceProtocol);
		break;
	}

	case TUSB_DESC_ENDPOINT: {
		auto ep = as<tusb_desc_endpoint_t>();
		n += p.println();
		n += p.print("  Addr ");
		n += p.print(String(ep->bEndpointAddress, HEX, 2));
		n += p.print(", xfer ");
		n += p.print(ep->bmAttributes.xfer);
		n += p.print(' ');
		n += p.print(getXferTypeName(ep->bmAttributes.xfer));
		n += p.print(", sync ");
		n += p.print(ep->bmAttributes.sync);
		n += p.print(", usage ");
		n += p.print(ep->bmAttributes.usage);
		n += p.print(", MaxPacketSize ");
		n += p.print(ep->wMaxPacketSize);
		n += p.print(", interval ");
		n += p.print(ep->bInterval);
		break;
	}

	default:;
	}

	return n;
}

size_t DescriptorList::printTo(Print& p) const
{
	size_t n{0};
	for(auto desc : *this) {
		n += p.println(*desc);
	}
	return n;
}

} // namespace USB
