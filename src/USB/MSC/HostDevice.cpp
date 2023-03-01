#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUH_MSC

#include <Storage/Disk.h>
#include <debug_progmem.h>
#include <Platform/WDT.h>

namespace USB::MSC
{
namespace
{
MountCallback mountCallback;
UnmountCallback unmountCallback;
HostDevice* host_devices[CFG_TUH_DEVICE_MAX];
} // namespace

void onMount(MountCallback callback)
{
	mountCallback = callback;
}

void onUnmount(UnmountCallback callback)
{
	unmountCallback = callback;
}

HostDevice* getDevice(uint8_t dev_addr)
{
	unsigned idx = dev_addr - 1;
	return (idx < ARRAY_SIZE(host_devices)) ? host_devices[idx] : nullptr;
}

LogicalUnit::LogicalUnit(HostDevice& device, uint8_t lun) : device(device), lun(lun)
{
	sectorCount = device.getSectorCount(lun);
	sectorSize = device.getBlockSize(lun);
	sectorSizeShift = Storage::getSizeBits(sectorSize);
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

bool HostDevice::begin(const Instance& inst)
{
	HostInterface::begin(inst);
	state = State::ready;
	debug_i("[MSC] Device %u (%s) mounted, max_lun %u", inst.dev_addr, inst.name, tuh_msc_get_maxlun(inst.dev_addr));
	return true;
}

bool HostDevice::enumerate(EnumCallback callback)
{
	if(inquiry) {
		debug_e("[MSC] Enumeration already in progress");
		return false;
	}

	for(auto& unit : units) {
		unit.reset();
	}

	if(!sendInquiry(0)) {
		return false;
	}

	enumCallback = callback;
	return true;
}

bool HostDevice::sendInquiry(uint8_t lun)
{
	auto callback = [](uint8_t dev_addr, const tuh_msc_complete_data_t* cb_data) {
		auto dev = reinterpret_cast<HostDevice*>(cb_data->user_arg);
		return dev ? dev->handleInquiry(cb_data) : false;
	};

	if(!inquiry) {
		inquiry.reset(new Inquiry{});
	}

	if(!tuh_msc_inquiry(inst.dev_addr, lun, &inquiry->resp, callback, reinterpret_cast<uintptr_t>(this))) {
		debug_e("tuh_msc_inquiry failed");
		return false;
	}

	return true;
}

bool HostDevice::handleInquiry(const tuh_msc_complete_data_t* cb_data)
{
	auto lun = cb_data->cbw->lun;
	if(cb_data->csw->status != 0) {
		debug_e("[MSC] Inquiry failed (addr %u, lun %u)", inst.dev_addr, lun);
	} else {
		debug_hex(DBG, "INQUIRY", inquiry.get(), sizeof(*inquiry));
		auto block_count = tuh_msc_get_block_count(inst.dev_addr, lun);
		debug_d("[MSC] Block count %u, size %u", block_count, tuh_msc_get_block_size(inst.dev_addr, lun));
		// Ignore any un-populated units
		if(block_count != 0) {
			auto& unit = units[lun];
			if(!unit) {
				unit.reset(new LogicalUnit(*this, lun));
			}
			Storage::Disk::scanPartitions(*unit);
			if(enumCallback) {
				enumCallback(*unit, *inquiry);
			}
		}
	}

	++lun;
	if(lun < MAX_LUN && lun < tuh_msc_get_maxlun(inst.dev_addr)) {
		if(sendInquiry(lun)) {
			return true;
		}
	}

	inquiry.reset();
	enumCallback = nullptr;
	return true;
}

void HostDevice::end()
{
	HostInterface::end();
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

	if(!tuh_msc_read10(inst.dev_addr, lun, dst, lba, size, callback, reinterpret_cast<uintptr_t>(this))) {
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

	if(!tuh_msc_write10(inst.dev_addr, lun, src, lba, size, callback, reinterpret_cast<uintptr_t>(this))) {
		return false;
	}

	state = State::busy;
	return true;
}

} // namespace USB::MSC

using namespace USB::MSC;

void tuh_msc_mount_cb(uint8_t dev_addr)
{
	debug_i("%s(%u)", __FUNCTION__, dev_addr);

	unsigned idx = dev_addr - 1;
	if(idx >= ARRAY_SIZE(host_devices)) {
		return;
	}
	if(mountCallback) {
		HostDevice::Instance inst{dev_addr, 0, ""};
		host_devices[idx] = mountCallback(inst);
	}
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
	debug_i("%s(%u)", __FUNCTION__, dev_addr);

	auto dev = getDevice(dev_addr);
	if(dev) {
		dev->end();
		host_devices[dev_addr] = nullptr;
	}
}

#endif
