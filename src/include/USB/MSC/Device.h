#pragma once

#include <Storage/Device.h>

namespace USB::MSC
{
bool registerDevice(Storage::Device* device, bool readOnly);
bool unregisterDevice(Storage::Device* device);

} // namespace USB::MSC
