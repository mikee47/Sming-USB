#pragma once

#include "Descriptors.h"

namespace USB
{
class DeviceInterface
{
public:
	DeviceInterface(uint8_t instance, const char* name) : inst(instance), name(name)
	{
	}

protected:
	uint8_t inst;
	const char* name;
};

} // namespace USB
