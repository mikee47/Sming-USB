#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUH_MSC

#include <Storage/Disk.h>
#include <debug_progmem.h>
#include <Platform/WDT.h>

namespace USB::MSC
{
HostDevice::MountCallback HostDevice::mountCallback;
HostDevice::UnmountCallback HostDevice::unmountCallback;
std::unique_ptr<Inquiry> HostDevice::inquiry;

HostDevice* getDevice(uint8_t dev_addr)
{
	extern HostDevice* host_devices[];
	--dev_addr;
	return (dev_addr < CFG_TUH_MSC) ? host_devices[dev_addr] : nullptr;
}

uint32_t LogicalUnit::getId() const
{
	return device.getAddress();
}

String LogicalUnit::getName() const
{
	String s;
	s += device.getName();
	s += '.';
	s += lun;
	return s;
}

size_t LogicalUnit::getBlockSize() const
{
	return device.getBlockSize(lun);
}

storage_size_t LogicalUnit::getSectorCount() const
{
	return device.getSectorCount(lun);
}

bool LogicalUnit::raw_sector_read(storage_size_t address, void* dst, size_t size)
{
	return device.read_sectors(lun, address, dst, size);
}

bool LogicalUnit::raw_sector_write(storage_size_t address, const void* src, size_t size)
{
	return device.write_sectors(lun, address, src, size);
}

bool LogicalUnit::LogicalUnit::raw_sector_erase_range(storage_size_t address, size_t size)
{
	return false;
}

bool LogicalUnit::raw_sync()
{
	return device.wait();
}

bool HostDevice::begin(uint8_t deviceAddress)
{
	address = deviceAddress;
	state = State::ready;
	debug_i("[MSC] Device %u (%s) mounted, max_lun %u", address, name, tuh_msc_get_maxlun(address));
	return sendInquiry(0);
}

bool HostDevice::sendInquiry(uint8_t lun)
{
	auto callback = [](uint8_t dev_addr, const tuh_msc_complete_data_t* cb_data) {
		auto dev = reinterpret_cast<HostDevice*>(cb_data->user_arg);
		auto lun = cb_data->cbw->lun;
		if(cb_data->csw->status != 0) {
			debug_e("[MSC] Inquiry failed (addr %u, lun %u)", dev_addr, lun);
		} else {
			debug_hex(DBG, "INQUIRY", inquiry.get(), sizeof(*inquiry));
			auto block_count = tuh_msc_get_block_count(dev_addr, lun);
			debug_d("[MSC] Block count %u, size %u", block_count, tuh_msc_get_block_size(dev_addr, lun));
			// Ignore any un-populated units
			if(block_count != 0) {
				auto& unit = dev->units[lun];
				if(!unit) {
					unit.reset(new LogicalUnit(*dev, lun));
				}
				Storage::Disk::scanPartitions(*unit);
				if(mountCallback) {
					mountCallback(*unit, *inquiry);
				}
			}
		}

		++lun;
		if(lun < MAX_LUN && lun < tuh_msc_get_maxlun(dev_addr)) {
			return dev->sendInquiry(lun);
		}

		inquiry.reset();
		return true;
	};

	if(!inquiry) {
		inquiry.reset(new Inquiry{});
	}

	if(!tuh_msc_inquiry(address, lun, &inquiry->resp, callback, reinterpret_cast<uintptr_t>(this))) {
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
	inquiry.reset();
	if(unmountCallback) {
		unmountCallback(*this);
	}
}

bool HostDevice::wait()
{
	while(state == State::busy) {
		tuh_task();
		system_soft_wdt_feed();
	}
	return state == State::ready;
}

bool HostDevice::read_sectors(uint8_t lun, uint32_t lba, void* dst, size_t size)
{
	if(state < State::ready) {
		return false;
	}

	auto callback = [](uint8_t dev_addr, tuh_msc_complete_data_t const* cb_data) {
		auto dev = reinterpret_cast<HostDevice*>(cb_data->user_arg);
		dev->state = State::ready;
		return true;
	};

	if(!tuh_msc_read10(address, lun, dst, lba, size, callback, reinterpret_cast<uintptr_t>(this))) {
		return false;
	}

	state = State::busy;
	return wait();
}

bool HostDevice::write_sectors(uint8_t lun, uint32_t lba, const void* src, size_t size)
{
	if(!wait()) {
		return false;
	}

	auto callback = [](uint8_t dev_addr, tuh_msc_complete_data_t const* cb_data) {
		auto dev = reinterpret_cast<HostDevice*>(cb_data->user_arg);
		dev->state = State::ready;
		return true;
	};

	if(!tuh_msc_write10(address, lun, src, lba, size, callback, reinterpret_cast<uintptr_t>(this))) {
		return false;
	}

	state = State::busy;
	return true;
}

} // namespace USB::MSC

using namespace USB::MSC;

void tuh_msc_mount_cb(uint8_t dev_addr)
{
	auto dev = getDevice(dev_addr);
	if(dev) {
		dev->begin(dev_addr);
	}
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
	auto dev = getDevice(dev_addr);
	if(dev) {
		dev->end();
	}
}

#endif
