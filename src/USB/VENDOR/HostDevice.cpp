#include <USB.h>

#if CFG_TUH_ENABLED && CFG_TUH_VENDOR

// #include <host/usbh.h>
// #include <class/vendor/vendor_host.h>

namespace USB::VENDOR
{
MountCallback mountCallback;
UnmountCallback unmountCallback;
HostDevice* host_devices[CFG_TUH_DEVICE_MAX];

void onMount(MountCallback callback)
{
	mountCallback = callback;
}

void onUnmount(UnmountCallback callback)
{
	unmountCallback = callback;
}

HostDevice* getDevice(uint8_t dev_addr, uint8_t ep_addr)
{
	// TODO: Multiple Vendor devices must be differentiated by ep_addr
	unsigned idx = dev_addr - 1;
	return (dev_addr < ARRAY_SIZE(host_devices)) ? host_devices[idx] : nullptr;
}

} // namespace USB::VENDOR

using namespace USB::VENDOR;
using namespace USB;

void cush_init(void)
{
	debug_i("%s()", __FUNCTION__);
}

bool cush_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const* itf_desc, uint16_t max_len)
{
	debug_i("%s(rhport %u, dev_addr %u, max_len %u)", __FUNCTION__, rhport, dev_addr, max_len);

	if(!mountCallback) {
		return false;
	}

	auto dev = getDevice(dev_addr, 0);
	if(dev) {
		debug_e("[USB-VEND] No free instances");
		return false;
	}

	DescriptorList list{reinterpret_cast<const Descriptor*>(itf_desc), max_len};
	dev = mountCallback({dev_addr, 0}, list);
	if(!dev) {
		return false;
	}

	host_devices[dev_addr - 1] = dev;
	return true;
}

bool cush_set_config(uint8_t dev_addr, uint8_t itf_num)
{
	debug_i("%s(dev_addr %u, itf_num %u)", __FUNCTION__, dev_addr, itf_num);

	auto dev = getDevice(dev_addr, 0);
	return dev && dev->setConfig(itf_num);
}

bool cush_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
	debug_i("%s(dev_addr %u, ep_addr %u, result %u, xferred_bytes %u)", __FUNCTION__, dev_addr, ep_addr, result,
			xferred_bytes);

	auto dev = getDevice(dev_addr, ep_addr);
	return dev && dev->transferComplete({dev_addr, ep_addr, result, xferred_bytes});
}

void cush_close(uint8_t dev_addr)
{
	debug_i("%s(dev_addr %u)", __FUNCTION__, dev_addr);

	auto dev = getDevice(dev_addr, 0);
	if(dev) {
		dev->end();
	}
}

#endif
