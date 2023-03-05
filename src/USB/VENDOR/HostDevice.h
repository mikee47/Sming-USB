#pragma once

#include "../HostInterface.h"
#include <debug_progmem.h>

namespace USB::VENDOR
{
class HostDevice : public HostInterface
{
public:
	struct Transfer {
		uint8_t dev_addr;
		uint8_t ep_addr;
		xfer_result_t result;
		uint32_t xferred_bytes;
	};

	virtual bool setConfig(uint8_t itf_num) = 0;
	virtual bool transferComplete(const Transfer& txfr) = 0;

	using HostInterface::HostInterface;
};

using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst, DescriptorEnum itf)>;
using UnmountCallback = Delegate<void(HostDevice& dev)>;

void onMount(MountCallback callback);
void onUnmount(UnmountCallback callback);

} // namespace USB::VENDOR
