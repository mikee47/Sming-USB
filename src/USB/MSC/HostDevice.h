#pragma once

#include <Storage/Disk/BlockDevice.h>

namespace USB::MSC
{
class HostDevice;
class LogicalUnit : public Storage::Disk::BlockDevice
{
public:
	LogicalUnit(HostDevice& device, uint8_t lun) : device(device), lun(lun)
	{
	}

	/* Storage Device methods */

	Type getType() const override
	{
		return Type::disk;
	}

	String getName() const override;
	uint32_t getId() const override;
	size_t getBlockSize() const override;
	storage_size_t getSectorCount() const override;

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

class HostDevice
{
public:
	using MountCallback = Delegate<void(LogicalUnit& unit, const Inquiry& inquiry)>;
	using UnmountCallback = Delegate<void(HostDevice& dev)>;

	HostDevice(uint8_t instance, const char* name) : name(name)
	{
	}

	~HostDevice()
	{
		end();
	}

	bool begin(uint8_t deviceAddress);

	void end();

	const char* getName() const
	{
		return name;
	}

	uint8_t getAddress() const
	{
		return address;
	}

	static void onMount(MountCallback callback)
	{
		mountCallback = callback;
	}

	static void onUnmount(UnmountCallback callback)
	{
		unmountCallback = callback;
	}

	LogicalUnit* operator[](unsigned lun) const
	{
		return (lun < MAX_LUN) ? units[lun].get() : nullptr;
	}

	size_t getBlockSize(uint8_t lun) const
	{
		return tuh_msc_get_block_size(address, lun);
	}

	storage_size_t getSectorCount(uint8_t lun) const
	{
		return tuh_msc_get_block_count(address, lun);
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

	static MountCallback mountCallback;
	static UnmountCallback unmountCallback;
	static std::unique_ptr<Inquiry> inquiry;

	State state{};
	const char* name;
	uint8_t address{0};
};

HostDevice* getDevice(uint8_t dev_addr);

} // namespace USB::MSC
