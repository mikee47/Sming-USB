#pragma once

#include <Storage/Device.h>

namespace USB::MSC
{
struct LogicalUnit {
	Storage::Device* device;
	bool readOnly;

	operator bool() const
	{
		return device != nullptr;
	}

	void getCapacity(uint32_t* block_count, uint16_t* block_size)
	{
		if(device == nullptr) {
			return;
		}

		*block_size = device->getSectorSize();
		*block_count = device->getSectorCount();
	}

	int read(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
	{
		if(device == nullptr) {
			return -1;
		}

		auto blockSize = device->getSectorSize();
		offset += blockSize * lba;
		return device->read(offset, buffer, bufsize) ? bufsize : -1;
	}

	int write(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
	{
		if(device == nullptr || readOnly) {
			return -1;
		}

		auto blockSize = device->getSectorSize();
		offset += blockSize * lba;
		return device->write(offset, buffer, bufsize) ? bufsize : -1;
	}
};
class Device
{
public:
	Device(uint8_t instance, const char* name)
	{
	}

	static bool add(Storage::Device* device, bool readOnly);
	static bool remove(const Storage::Device* device);

	static void getCapacity(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
	{
		return getLogicalUnit(lun).getCapacity(block_count, block_size);
	}

	static int read(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
	{
		return getLogicalUnit(lun).read(lba, offset, buffer, bufsize);
	}

	static int write(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
	{
		return getLogicalUnit(lun).write(lba, offset, buffer, bufsize);
	}

	static void inquiry(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]);

	static bool isReady(uint8_t lun)
	{
		return getLogicalUnit(lun);
	}

	static bool isReadOnly(uint8_t lun)
	{
		return getLogicalUnit(lun).readOnly;
	}

	static constexpr size_t MAX_LUN{4};

private:
	static LogicalUnit getLogicalUnit(uint8_t lun)
	{
		return (lun < MAX_LUN) ? logicalUnits[lun] : LogicalUnit{};
	}

	static LogicalUnit logicalUnits[];
};

} // namespace USB::MSC
