#pragma once

#include "../Interface.h"

namespace USB::HID
{
class HostDevice: public Interface
{
public:
	using Interface::Interface;
};

} // namespace USB::MSC
