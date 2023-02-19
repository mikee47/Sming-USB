#pragma once

#include <Storage/Disk/BlockDevice.h>

namespace USB::MSC
{
class HostDevice : public Storage::Disk::BlockDevice
{
public:
	/*
	 * Whilst SD V1.XX permits misaligned and partial block reads, later versions do not
	 * and require transfers to be aligned to, and in multiples of, 512 bytes.
	 */
	HostDevice(const String& name) : BlockDevice(), name(name)
	{
	}

	~HostDevice()
	{
		end();
	}

	bool begin(uint8_t deviceAddress);

	void end();

	/* Storage Device methods */

	String getName() const override
	{
		return name.c_str();
	}

	uint32_t getId() const override
	{
		return deviceAddress;
	}

	Type getType() const override
	{
		return Type::disk;
	}

	size_t getBlockSize() const override
	{
		return block_size;
	}

protected:
	bool raw_sector_read(storage_size_t address, void* dst, size_t size) override;
	bool raw_sector_write(storage_size_t address, const void* src, size_t size) override;
	bool raw_sector_erase_range(storage_size_t address, size_t size) override;
	bool raw_sync() override;

private:
	enum class State {
		idle,
		initialising,
		ready,
		busy,
	};

	bool wait();

	CString name;
	uint32_t block_size{0};
	uint32_t block_count{0};
	uint8_t deviceAddress{0};
	State state{};
};

} // namespace USB
