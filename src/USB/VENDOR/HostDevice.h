#pragma once

#include "../HostInterface.h"
#include <debug_progmem.h>

namespace USB::VENDOR
{
class HostDevice : public HostInterface
{
public:
	using HostInterface::HostInterface;
};

using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst)>;
using UnmountCallback = Delegate<void(HostDevice& dev)>;

// void onMount(MountCallback callback);
// void onUnmount(UnmountCallback callback);

} // namespace USB::VENDOR
