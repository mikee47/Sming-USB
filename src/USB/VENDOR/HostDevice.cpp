/****
 * VENDOR/HostDevice.cpp
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

#include <USB.h>

#if CFG_TUH_ENABLED && CFG_TUH_VENDOR

namespace USB::VENDOR
{
MountCallback mountCallback;
UnmountCallback unmountCallback;
HostDevice* host_devices[CFG_TUH_VENDOR];

uint8_t getEpIndex(uint8_t ep_addr)
{
	return (ep_addr & 0x0f) | ((ep_addr & 0x80) >> 3);
}

void onMount(MountCallback callback)
{
	mountCallback = callback;
}

void onUnmount(UnmountCallback callback)
{
	unmountCallback = callback;
}

HostDevice* getDeviceByInterface(uint8_t dev_addr, uint8_t itf_num)
{
	for(auto dev : host_devices) {
		if(dev && *dev == HostInterface::Instance{dev_addr, itf_num}) {
			return dev;
		}
	}
	return nullptr;
}

HostDevice* getDeviceByEndpoint(uint8_t dev_addr, uint8_t ep_addr)
{
	for(auto dev : host_devices) {
		if(dev && dev->getAddress() == dev_addr && dev->ownsEndpoint(ep_addr)) {
			return dev;
		}
	}
	return nullptr;
}

bool HostDevice::ownsEndpoint(uint8_t ep_addr)
{
	return ep_mask[getEpIndex(ep_addr)];
}

bool HostDevice::openEndpoint(const tusb_desc_endpoint_t& ep_desc)
{
	TU_ASSERT(tuh_edpt_open(inst.dev_addr, &ep_desc));
	ep_mask.set(getEpIndex(ep_desc.bEndpointAddress));
	return true;
}

} // namespace USB::VENDOR

using namespace USB::VENDOR;
using namespace USB;

void cush_init(void)
{
	debug_i("%s()", __FUNCTION__);
}

bool cush_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const* itf_desc, uint16_t max_len)
{
	debug_d("%s(rhport %u, dev_addr %u, itf_num %u, max_len %u)", __FUNCTION__, rhport, dev_addr,
			itf_desc->bInterfaceNumber, max_len);

	if(!mountCallback) {
		return false;
	}

	unsigned dev_idx = dev_addr - 1;
	TU_ASSERT(dev_idx < CFG_TUH_DEVICE_MAX, false);

	auto getFreeDevice = [&]() -> HostDevice** {
		for(auto& dev : host_devices) {
			if(!dev) {
				return &dev;
			}
		}
		return nullptr;
	};

	auto freeDevPtr = getFreeDevice();
	if(!freeDevPtr) {
		debug_e("[USB-VEND] No free instances");
		return false;
	}

	HostInterface::Instance inst{dev_addr, itf_desc->bInterfaceNumber};
	DescriptorList list{reinterpret_cast<const Descriptor*>(itf_desc), max_len};
	HostDevice::Config cfg{.list = list};
	tuh_vid_pid_get(dev_addr, &cfg.vid, &cfg.pid);
	auto dev = mountCallback(inst, cfg);
	*freeDevPtr = dev;
	return dev;
}

bool cush_set_config(uint8_t dev_addr, uint8_t itf_num)
{
	debug_d("%s(dev_addr %u, itf_num %u)", __FUNCTION__, dev_addr, itf_num);

	auto dev = getDeviceByInterface(dev_addr, itf_num);
	return dev && dev->setConfig(itf_num);
}

bool cush_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
	debug_d("%s(dev_addr %u, ep_addr %u, result %u, xferred_bytes %u)", __FUNCTION__, dev_addr, ep_addr, result,
			xferred_bytes);

	auto dev = getDeviceByEndpoint(dev_addr, ep_addr);
	return dev && dev->transferComplete({dev_addr, ep_addr, result, xferred_bytes});
}

void cush_close(uint8_t dev_addr)
{
	debug_d("%s(dev_addr %u)", __FUNCTION__, dev_addr);

	for(auto& dev : host_devices) {
		if(dev && dev->getAddress() == dev_addr) {
			dev->end();
			dev = nullptr;
		}
	}
}

#endif
