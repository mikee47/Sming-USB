#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUH_CDC

namespace USB::CDC
{
HostDevice::MountCallback HostDevice::mountCallback;
HostDevice::UnmountCallback HostDevice::unmountCallback;

class InternalHostDevice : public HostDevice
{
public:
	using HostDevice::begin;
	using HostDevice::end;
};

InternalHostDevice* getDevice(uint8_t inst)
{
	extern InternalHostDevice* host_devices[];
	return (inst < CFG_TUH_CDC) ? host_devices[inst] : nullptr;
}

HostDevice::HostDevice(uint8_t instance, const char* name) : UsbSerial(instance, name)
{
}

size_t HostDevice::write(const uint8_t* buffer, size_t size)
{
	size_t written{0};
	while(size != 0) {
		size_t n = tuh_cdc_write_available(inst);
		if(n == 0) {
			tuh_cdc_write_flush(inst);
		} else {
			n = std::min(n, size);
			tuh_cdc_write(inst, buffer, n);
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

void tuh_cdc_mount_cb(uint8_t inst)
{
	auto dev = getDevice(inst);
	if(dev) {
		dev->begin();
	}
}

void tuh_cdc_umount_cb(uint8_t inst)
{
	auto dev = getDevice(inst);
	if(dev) {
		dev->end();
	}
}

void tuh_cdc_rx_cb(uint8_t inst)
{
	auto dev = getDevice(inst);
	if(dev) {
		dev->handleEvent(Event::rx_data);
	}
}

void tuh_cdc_tx_complete_cb(uint8_t inst)
{
	auto dev = getDevice(inst);
	if(dev) {
		dev->handleEvent(Event::tx_done);
	}
}

#endif
