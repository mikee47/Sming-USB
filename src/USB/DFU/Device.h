#pragma once

#include "../Interface.h"

namespace USB::DFU
{
class Device : public Interface
{
public:
	Device(uint8_t inst, const char* name);
};

} // namespace USB::DFU
