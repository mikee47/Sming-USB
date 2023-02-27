#pragma once

#include "../Interface.h"

namespace USB::HID
{
using Report = DescriptorList;

class HostDevice : public Interface
{
public:
	using MountCallback = Delegate<void(HostDevice& dev, const Report& report)>;
	using UnmountCallback = Delegate<void(HostDevice& dev)>;
	using ReportReceived = Delegate<void(const Report& report)>;

	HostDevice(uint8_t inst, const char* name);

	static void onMount(MountCallback callback)
	{
		mountCallback = callback;
	}

	static void onUnmount(UnmountCallback callback)
	{
		unmountCallback = callback;
	}

	bool requestReport(ReportReceived callback);

protected:
	void begin(DescriptorList report)
	{
		if(mountCallback) {
			mountCallback(*this, report);
		}
	}

	void end()
	{
		if(unmountCallback) {
			unmountCallback(*this);
		}
	}

	void reportReceived(DescriptorList report)
	{
		if(reportReceivedCallback) {
			reportReceivedCallback(report);
		}
	}

private:
	static MountCallback mountCallback;
	static UnmountCallback unmountCallback;
	ReportReceived reportReceivedCallback;
};

} // namespace USB::HID
