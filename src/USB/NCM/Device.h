#pragma once

#include "../DeviceInterface.h"

namespace USB::NCM
{
class Device : public DeviceInterface
{
public:
	using Interface::DeviceInterface;
};

} // namespace USB::NCM
