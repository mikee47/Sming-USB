#pragma once

#include "../DeviceInterface.h"

namespace USB::ECM_RNDIS
{
class Device : public DeviceInterface
{
public:
	using DeviceInterface::DeviceInterface;
};

} // namespace USB::ECM_RNDIS
