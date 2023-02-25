#pragma once

#include "../Interface.h"

namespace USB::DFU
{
class Device : public Interface
{
public:
	using Interface::Interface;
};

} // namespace USB::DFU
