#include <USB.h>

#ifdef CFG_TUH_MSC

#include <Storage/Disk.h>
#include <debug_progmem.h>
#include <Platform/WDT.h>

namespace
{
scsi_inquiry_resp_t inquiry_resp;
constexpr uint8_t lun_default{0};
}; // namespace

namespace USB::MSC
{
HostDevice* devices[CFG_TUH_DEVICE_MAX];

bool HostDevice::begin(uint8_t deviceAddress)
{
	if(deviceAddress == 0 || deviceAddress > CFG_TUH_DEVICE_MAX) {
		debug_e("[MSC] Bad device address %u", deviceAddress);
		return false;
	}
	if(devices[deviceAddress - 1]) {
		debug_e("[MSC] Device %u already initialised", deviceAddress);
		return false;
	}
	this->deviceAddress = deviceAddress;
	devices[deviceAddress - 1] = this;
	state = State::initialising;

	auto callback = [](uint8_t dev_addr, const tuh_msc_complete_data_t* cb_data) {
		if(cb_data->csw->status != 0) {
			debug_e("Inquiry failed");
			return false;
		}

		auto dev = reinterpret_cast<HostDevice*>(cb_data->user_arg);

		// Print out Vendor ID, Product ID and Rev
		debug_i("%.8s %.16s rev %.4s", inquiry_resp.vendor_id, inquiry_resp.product_id, inquiry_resp.product_rev);

		// Get capacity of device
		dev->block_size = tuh_msc_get_block_size(dev_addr, cb_data->cbw->lun);
		dev->block_count = tuh_msc_get_block_count(dev_addr, cb_data->cbw->lun);

		debug_i("Disk Size %lu MB", dev->block_count / ((1024 * 1024) / dev->block_size));
		debug_i("Block Count %lu, Block Size %lu", dev->block_count, dev->block_size);

		dev->state = State::ready;
		return true;
	};

	if(!tuh_msc_inquiry(deviceAddress, lun_default, &inquiry_resp, callback, reinterpret_cast<uintptr_t>(this))) {
		debug_e("tuh_msc_inquiry failed");
		end();
		return false;
	}

	while(state == State::initialising) {
		tuh_task();
	}

	debug_i("Scanning partitions");
	Storage::Disk::scanPartitions(*this);

	return true;
}

void HostDevice::end()
{
	if(state == State::idle) {
		return;
	}
	assert(deviceAddress != 0 && deviceAddress <= CFG_TUH_DEVICE_MAX);
	devices[deviceAddress - 1] = nullptr;
	state = State::idle;
	deviceAddress = 0;
	block_size = 0;
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

} // namespace USB

#endif
