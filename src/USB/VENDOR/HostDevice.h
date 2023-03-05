#pragma once

#include "../HostInterface.h"
#include <debug_progmem.h>

namespace USB::VENDOR
{
class HostDevice : public HostInterface
{
public:
	struct Config {
		uint16_t vid;
		uint16_t pid;
		DescriptorList list;
	};

	struct Transfer {
		uint8_t dev_addr;
		uint8_t ep_addr;
		xfer_result_t result;
		uint32_t xferred_bytes;
	};

	virtual bool setConfig(uint8_t itf_num) = 0;
	virtual bool ownsEndpoint(uint8_t ep) = 0;
	virtual bool transferComplete(const Transfer& txfr) = 0;

	using HostInterface::HostInterface;
};

using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst, const HostDevice::Config& cfg)>;
using UnmountCallback = Delegate<void(HostDevice& dev)>;

void onMount(MountCallback callback);
void onUnmount(UnmountCallback callback);

} // namespace USB::VENDOR
