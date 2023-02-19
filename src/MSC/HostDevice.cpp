#include <USB.h>

#ifdef CFG_TUH_MSC

#include <Storage/Disk.h>
#include <debug_progmem.h>
#include <Platform/WDT.h>

namespace
{
constexpr uint8_t lun_default{0};
}

namespace USB::MSC
{
HostDevice::MountCallback HostDevice::mountCallback;
HostDevice::UnmountCallback HostDevice::unmountCallback;

HostDevice* getDevice(uint8_t dev_addr)
{
	extern HostDevice* devices[];
	--dev_addr;
	return (dev_addr < CFG_TUH_MSC) ? devices[dev_addr] : nullptr;
}

} // namespace USB::MSC

void tuh_msc_mount_cb(uint8_t dev_addr)
{
	auto dev = USB::MSC::getDevice(dev_addr);
	if(dev) {
		dev->begin(dev_addr);
	}
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
	auto dev = USB::MSC::getDevice(dev_addr);
	if(dev) {
		dev->end();
	}
}

namespace USB::MSC
{
bool HostDevice::begin(uint8_t deviceAddress)
{
	debug_i("[MSC] Device %u (%s) mounted", deviceAddress, getName().c_str());
	this->deviceAddress = deviceAddress;
	block_size = tuh_msc_get_block_size(deviceAddress, lun_default);
	block_count = tuh_msc_get_block_count(deviceAddress, lun_default);

	static std::unique_ptr<scsi_inquiry_resp_t> inquiry_resp;
	auto callback = [](uint8_t dev_addr, const tuh_msc_complete_data_t* cb_data) {
		if(cb_data->csw->status != 0) {
			debug_e("Inquiry failed");
		} else {
			auto dev = reinterpret_cast<HostDevice*>(cb_data->user_arg);
			dev->state = State::ready;
			Storage::Disk::scanPartitions(*dev);
			if(mountCallback) {
				mountCallback(*dev, *inquiry_resp);
			}
		}
		inquiry_resp.reset();
		return true;
	};

	inquiry_resp.reset(new scsi_inquiry_resp_t);
	if(!tuh_msc_inquiry(deviceAddress, lun_default, inquiry_resp.get(), callback, reinterpret_cast<uintptr_t>(this))) {
		debug_e("tuh_msc_inquiry failed");
		end();
		return false;
	}

	return true;
}

void HostDevice::end()
{
	if(state == State::idle) {
		return;
	}
	wait();
	state = State::idle;
	if(unmountCallback) {
		unmountCallback(*this);
	}
	deviceAddress = 0;
	block_count = 0;
}

bool HostDevice::wait()
{
	while(state == State::busy) {
		tuh_task();
		system_soft_wdt_feed();
	}
	return state == State::ready;
}

bool HostDevice::raw_sector_read(storage_size_t address, void* dst, size_t size)
{
	if(state < State::ready) {
		return false;
	}

	auto callback = [](uint8_t dev_addr, tuh_msc_complete_data_t const* cb_data) {
		auto dev = reinterpret_cast<HostDevice*>(cb_data->user_arg);
		dev->state = State::ready;
		return true;
	};

	if(!tuh_msc_read10(deviceAddress, lun_default, dst, address, size, callback, reinterpret_cast<uintptr_t>(this))) {
		return false;
	}

	state = State::busy;
	return wait();
}

bool HostDevice::raw_sector_write(storage_size_t address, const void* src, size_t size)
{
	if(!wait()) {
		return false;
	}

	auto callback = [](uint8_t dev_addr, tuh_msc_complete_data_t const* cb_data) {
		auto dev = reinterpret_cast<HostDevice*>(cb_data->user_arg);
		dev->state = State::ready;
		return true;
	};

	if(!tuh_msc_write10(deviceAddress, lun_default, src, address, size, callback, reinterpret_cast<uintptr_t>(this))) {
		return false;
	}

	state = State::busy;
	return true;
}

bool HostDevice::raw_sector_erase_range(storage_size_t address, size_t size)
{
	return false;
}

bool HostDevice::raw_sync()
{
	return wait();
}

} // namespace USB::MSC

#endif
