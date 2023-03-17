/****
 * MSC/HostDevice.h
 *
 * Copyright 2023 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the Sming USB Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

#pragma once

#include "../HostInterface.h"
#include <Storage/Disk/BlockDevice.h>

namespace USB::MSC
{
class HostDevice;

/**
 * @brief A physical device instance managed by an MSC interface.
 */
class LogicalUnit : public Storage::Disk::BlockDevice
{
public:
	LogicalUnit(HostDevice& device, uint8_t lun);

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

/**
 * @brief Information provided by SCSI inquiry operation
 */
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

/**
 * @brief A USB mass storage device supports one or more logical units,
 * each of which is a physical storage device.
 */
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

	/**
	 * @brief Enumerate all logical units managed by this device
	 * @param callback Invoked for each discovered Logical Unit
	 */
	bool enumerate(EnumCallback callback);

	/**
	 * @brief Access a specific logical unit by number
	 * @param lun The logical Unit Number
	 * @retval LogicalUnit* The corresponding LU, or nullptr if invalid
	 */
	LogicalUnit* operator[](unsigned lun) const
	{
		return (lun < MAX_LUN) ? units[lun].get() : nullptr;
	}

	/**
	 * @brief Get the declared block/sector size for a unit
	 * @param lun The logical Unit Number
	 * @retval size_t Block size in bytes, or 0 if invalid
	 */
	size_t getSectorSize(uint8_t lun) const
	{
		return tuh_msc_get_block_size(inst.dev_addr, lun);
	}

	/**
	 * @brief Get the number of blocks/sectors for a unit
	 * @param lun The logical Unit Number
	 * @retval size_t Number of blocks, 0 if invalid
	 */
	storage_size_t getSectorCount(uint8_t lun) const
	{
		return tuh_msc_get_block_count(inst.dev_addr, lun);
	}

	/**
	 * @brief Read data from a unit
	 * @param lun The logical Unit Number
	 * @param lba Starting Logical Block Address
	 * @param dst Buffer to store data
	 * @param size Number of sectors to read
	 * @retval bool true on success
	 */
	bool read_sectors(uint8_t lun, uint32_t lba, void* dst, size_t size);

	/**
	 * @brief Write data to a unit
	 * @param lun The logical Unit Number
	 * @param lba Starting Logical Block Address
	 * @param src Data to write
	 * @param size Number of sectors to write
	 * @retval bool true on success
	 */
	bool write_sectors(uint8_t lun, uint32_t lba, const void* src, size_t size);

	/**
	 * @brief Wait for all outstanding operations to complete
	 * @param lun The logical Unit Number
	 * @retval bool false on error (e.g. device forceably disconnected)
	 *
	 * Write operations are asynchronous so calling this method ensures that the operation
	 * has completed.
	 */
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

/**
 * @brief Application callback to notify connection of a new device
 * @param inst TinyUSB device instance
 * @retval HostDevice* Application returns pointer to implementation, or nullptr to ignore this device
 */
using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst)>;

/**
 * @brief Application callback to notify disconnection of a device
 * @param dev The device which has been disconnected
 */
using UnmountCallback = Delegate<void(HostDevice& dev)>;

/**
 * @brief Application should call this method to receive device connection notifications
 * @param callback
 */
void onMount(MountCallback callback);

/**
 * @brief Application should call this method to receive device disconnection notifications
 * @param callback
 */
void onUnmount(UnmountCallback callback);

} // namespace USB::MSC
