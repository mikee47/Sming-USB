#pragma once

#include "../HostInterface.h"
#include <Storage/Disk/BlockDevice.h>

namespace USB::MSC
{
class HostDevice;
class LogicalUnit : public Storage::Disk::BlockDevice
{
public:
	LogicalUnit(HostDevice& device, uint8_t lun);

	/* Storage Device methods */

	Type getType() const override
	{
		return Type::disk;
	}

	String getName() const override;
	uint32_t getId() const override;

protected:
	bool raw_sector_read(storage_size_t address, void* dst, size_t size) override;
	bool raw_sector_write(storage_size_t address, const void* src, size_t size) override;
	bool raw_sector_erase_range(storage_size_t address, size_t size) override;
	bool raw_sync() override;

private:
	HostDevice& device;
	uint8_t lun;
};

struct Inquiry {
	scsi_inquiry_resp_t resp;

	String vendorId() const
	{
		return String(reinterpret_cast<const char*>(resp.vendor_id), sizeof(resp.vendor_id));
	}

	String productId() const
	{
		return String(reinterpret_cast<const char*>(resp.product_id), sizeof(resp.product_id));
	}

	String productRev() const
	{
		return String(reinterpret_cast<const char*>(resp.product_rev), sizeof(resp.product_rev));
	}
};

class HostDevice : public HostInterface
{
public:
	/**
	  * @brief Callback passed to enumerate() method
	  * @param unit The logical unit attached to a device
	  * @param inquiry Detailed information
	  * @retval bool true to continue enumeration, false to stop
	  */
	using EnumCallback = Delegate<bool(LogicalUnit& unit, const Inquiry& inquiry)>;

	using HostInterface::HostInterface;

	bool begin(const Instance& inst);
	void end();

	bool enumerate(EnumCallback callback);

	LogicalUnit* operator[](unsigned lun) const
	{
		return (lun < MAX_LUN) ? units[lun].get() : nullptr;
	}

	size_t getBlockSize(uint8_t lun) const
	{
		return tuh_msc_get_block_size(inst.dev_addr, lun);
	}

	storage_size_t getSectorCount(uint8_t lun) const
	{
		return tuh_msc_get_block_count(inst.dev_addr, lun);
	}

	bool read_sectors(uint8_t lun, uint32_t lba, void* dst, size_t size);
	bool write_sectors(uint8_t lun, uint32_t lba, const void* src, size_t size);
	bool wait();

	static constexpr size_t MAX_LUN{CFG_TUH_MSC_MAXLUN};
	std::unique_ptr<LogicalUnit> units[MAX_LUN]{};

private:
	enum class State {
		idle,
		ready,
		busy,
	};

	bool sendInquiry(uint8_t lun);
	bool handleInquiry(const tuh_msc_complete_data_t* cb_data);

	std::unique_ptr<Inquiry> inquiry;
	EnumCallback enumCallback;
	State state{};
};

using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst)>;
using UnmountCallback = Delegate<void(HostDevice& dev)>;

void onMount(MountCallback callback);
void onUnmount(UnmountCallback callback);

} // namespace USB::MSC
