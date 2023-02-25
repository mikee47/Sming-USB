#pragma once

#include "Descriptors.h"

namespace USB
{
class Interface
{
public:
	Interface(uint8_t instance, const char* name) : inst(instance)
	{
	}

protected:
	uint8_t inst;
};

} // namespace USB
