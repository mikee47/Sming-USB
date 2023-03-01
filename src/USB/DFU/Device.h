#pragma once

#include "../DeviceInterface.h"

namespace USB::DFU
{
class Device : public DeviceInterface
{
public:
	Device(uint8_t inst, const char* name);
};

} // namespace USB::DFU
