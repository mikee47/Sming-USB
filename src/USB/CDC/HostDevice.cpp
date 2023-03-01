#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUH_CDC

namespace USB::CDC
{
namespace
{
MountCallback mountCallback;
UnmountCallback unmountCallback;
HostDevice* host_devices[CFG_TUH_CDC];
} // namespace

void onMount(MountCallback callback)
{
	mountCallback = callback;
}

void onUnmount(UnmountCallback callback)
{
	unmountCallback = callback;
}

HostDevice* getDevice(uint8_t idx)
{
	return (idx < CFG_TUH_CDC) ? host_devices[idx] : nullptr;
}

size_t HostDevice::write(const uint8_t* buffer, size_t size)
{
	size_t written{0};
	while(size != 0) {
		size_t n = tuh_cdc_write_available(inst.idx);
		if(n == 0) {
			tuh_cdc_write_flush(inst.idx);
		} else {
			n = std::min(n, size);
			tuh_cdc_write(inst.idx, buffer, n);
			written += n;
			buffer += n;
			size -= n;
		}
		if(!bitRead(options, UART_OPT_TXWAIT)) {
			break;
		}
		tuh_task_ext(0, true);
	}

	flushTimer.startOnce();

	return written;
}

} // namespace USB::CDC

using namespace USB::CDC;

void tuh_cdc_mount_cb(uint8_t idx)
{
	if(idx >= ARRAY_SIZE(host_devices)) {
		return;
	}
	if(mountCallback) {
		USB::HostInterface::Instance inst{0, idx, ""};
		host_devices[idx] = mountCallback(inst);
	}
}

void tuh_cdc_umount_cb(uint8_t idx)
{
	auto dev = getDevice(idx);
	if(!dev) {
		return;
	}
	dev->end();
	if(unmountCallback) {
		unmountCallback(*dev);
	}
	host_devices[idx] = nullptr;
}

void tuh_cdc_rx_cb(uint8_t idx)
{
	auto dev = getDevice(idx);
	if(dev) {
		dev->handleEvent(Event::rx_data);
	}
}

void tuh_cdc_tx_complete_cb(uint8_t idx)
{
	auto dev = getDevice(idx);
	if(dev) {
		dev->handleEvent(Event::tx_done);
	}
}

#endif
