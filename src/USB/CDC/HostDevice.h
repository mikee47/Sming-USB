#pragma once

#include "UsbSerial.h"

namespace USB::CDC
{
class HostDevice : public UsbSerial
{
public:
	using MountCallback = Delegate<void(HostDevice& dev)>;
	using UnmountCallback = Delegate<void(HostDevice& dev)>;

	HostDevice(uint8_t instance, const char* name);

	static void onMount(MountCallback callback)
	{
		mountCallback = callback;
	}

	static void onUnmount(UnmountCallback callback)
	{
		unmountCallback = callback;
	}

	size_t setRxBufferSize(size_t size) override
	{
		return CFG_TUH_CDC_RX_BUFSIZE;
	}

	virtual size_t setTxBufferSize(size_t size) override
	{
		return CFG_TUH_CDC_TX_BUFSIZE;
	}

	int available() override
	{
		return tuh_cdc_read_available(inst);
	}

	int read() override
	{
		char c;
		return tuh_cdc_read(inst, &c, 1) ? c : -1;
	}

	size_t readBytes(char* buffer, size_t length) override
	{
		return tuh_cdc_read(inst, buffer, length);
	}

	int peek() override
	{
		uint8_t c;
		return tuh_cdc_peek(inst, &c) ? c : -1;
	}

	void clear(SerialMode mode = SERIAL_FULL) override
	{
		if(mode != SerialMode::TxOnly) {
			tuh_cdc_read_clear(inst);
		}
		if(mode != SerialMode::RxOnly) {
			tuh_cdc_write_clear(inst);
		}
	}

	void flush() override
	{
		tuh_cdc_write_flush(inst);
	}

	using Stream::write;

	size_t write(const uint8_t* buffer, size_t size) override;

protected:
	void begin()
	{
		if(mountCallback) {
			mountCallback(*this);
		}
	}

	void end()
	{
		if(unmountCallback) {
			unmountCallback(*this);
		}
	}

private:
	static MountCallback mountCallback;
	static UnmountCallback unmountCallback;
};

} // namespace USB::CDC
