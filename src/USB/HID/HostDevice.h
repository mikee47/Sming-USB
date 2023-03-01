#pragma once

#include "../HostInterface.h"
#include <debug_progmem.h>

namespace USB::HID
{
using Report = DescriptorList;

class HostDevice : public HostInterface
{
public:
	using ReportReceived = Delegate<void(const Report& report)>;

	using HostInterface::HostInterface;

	bool requestReport()
	{
		return tuh_hid_receive_report(inst.dev_addr, inst.idx);
	}

	void onReport(ReportReceived callback)
	{
		reportReceivedCallback = callback;
	}

	void reportReceived(DescriptorList report)
	{
		if(reportReceivedCallback) {
			reportReceivedCallback(report);
		}
	}

private:
	ReportReceived reportReceivedCallback;
};

using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst, const Report& report)>;
using UnmountCallback = Delegate<void(HostDevice& dev)>;

void onMount(MountCallback callback);
void onUnmount(UnmountCallback callback);

} // namespace USB::HID
