#pragma once

#include <tusb.h>

#ifdef ENABLE_USB_CLASSES
#include <usb_classdefs.h>
#endif

namespace USB
{
using GetDeviceDescriptor = Delegate<const tusb_desc_device_t*(const tusb_desc_device_t& desc)>;
using GetDescriptorString = Delegate<const uint16_t*(uint8_t index)>;

bool begin();

void onGetDeviceDescriptor(GetDeviceDescriptor callback);
void onGetDescriptorSting(GetDescriptorString callback);

} // namespace USB
