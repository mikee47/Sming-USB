#pragma once

#include "../HostInterface.h"
#include <debug_progmem.h>
#include <bitset>

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

	using HostInterface::HostInterface;

	void end() override
	{
		ep_mask.reset();
	}

	virtual bool setConfig(uint8_t itf_num) = 0;
	virtual bool transferComplete(const Transfer& txfr) = 0;

	bool ownsEndpoint(uint8_t ep_addr);

protected:
	bool openEndpoint(const tusb_desc_endpoint_t& ep_desc);

private:
	std::bitset<32> ep_mask{};
};

using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst, const HostDevice::Config& cfg)>;
using UnmountCallback = Delegate<void(HostDevice& dev)>;

void onMount(MountCallback callback);
void onUnmount(UnmountCallback callback);

} // namespace USB::VENDOR
