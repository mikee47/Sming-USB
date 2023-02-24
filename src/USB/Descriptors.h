#pragma once

#include <tusb.h>
#include <Delegate.h>
#include <usb_classdefs.h>

namespace USB
{
using GetDeviceDescriptor = Delegate<const tusb_desc_device_t*(const tusb_desc_device_t& desc)>;
using GetDescriptorString = Delegate<const uint16_t*(uint8_t index)>;

void onGetDeviceDescriptor(GetDeviceDescriptor callback);
void onGetDescriptorSting(GetDescriptorString callback);

} // namespace USB
