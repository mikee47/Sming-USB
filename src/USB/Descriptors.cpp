#include "USB.h"
#include <Platform/System.h>

namespace
{
USB::GetDeviceDescriptor deviceDescriptorCallback;
USB::GetDescriptorString descriptorStringCallback;

} // namespace

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

} // namespace USB
