#include <USB.h>

#if CFG_TUH_ENABLED && CFG_TUH_HID

namespace USB::HID
{
MountCallback mountCallback;
UnmountCallback unmountCallback;
HostDevice* host_devices[CFG_TUH_HID];

void onMount(MountCallback callback)
{
	mountCallback = callback;
}

void onUnmount(UnmountCallback callback)
{
	unmountCallback = callback;
}

class InternalHostDevice : public HostDevice
{
public:
	using HostDevice::begin;
	using HostDevice::end;
	using HostDevice::reportReceived;
};

InternalHostDevice* getDevice(uint8_t inst)
{
	return (inst < CFG_TUH_HID) ? static_cast<InternalHostDevice*>(host_devices[inst]) : nullptr;
}

} // namespace USB::HID

using namespace USB::HID;

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report_desc, uint16_t desc_len)
{
	if(!mountCallback) {
		return;
	}

	HostDevice::Instance inst{dev_addr, instance, "hid"};
	Report report{reinterpret_cast<const USB::Descriptor*>(report_desc), desc_len};
	host_devices[instance] = mountCallback(inst, report);
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
	auto dev = getDevice(instance);
	if(!dev) {
		return;
	}

	dev->end();
	if(unmountCallback) {
		unmountCallback(*dev);
	}
}

// Invoked when received report from device via interrupt endpoint
// Note: if there is report ID (composite), it is 1st byte of report
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
	auto dev = getDevice(instance);
	if(dev) {
		dev->reportReceived(Report{reinterpret_cast<const USB::Descriptor*>(report), len});
	}
}

// Invoked when sent report to device successfully via interrupt endpoint
void tuh_hid_report_sent_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
}

// Invoked when Sent Report to device via either control endpoint
// len = 0 indicate there is error in the transfer e.g stalled response
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type,
									uint16_t len)
{
}

// Invoked when Set Protocol request is complete
void tuh_hid_set_protocol_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t protocol)
{
}

#endif
