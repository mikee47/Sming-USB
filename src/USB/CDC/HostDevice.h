#pragma once

#include "UsbSerial.h"

namespace USB::CDC
{
class HostDevice : public UsbSerial
{
public:
	HostDevice(uint8_t instance, const char* name);

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
};

} // namespace USB::CDC
