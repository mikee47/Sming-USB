#pragma once

#include "UsbSerial.h"
#include "../DeviceInterface.h"

namespace USB::CDC
{
/**
 * @brief Serial device implementation, in ACM mode
 */
class Device : public DeviceInterface, public UsbSerial
{
public:
	Device(uint8_t idx, const char* name);

	size_t setRxBufferSize(size_t size) override
	{
		return CFG_TUD_CDC_RX_BUFSIZE;
	}

	virtual size_t setTxBufferSize(size_t size) override
	{
		return CFG_TUD_CDC_TX_BUFSIZE;
	}

	int available() override
	{
		return tud_cdc_n_available(inst);
	}

	bool isFinished() override
	{
		return !tud_cdc_n_connected(inst);
	}

	int read() override
	{
		return tud_cdc_n_read_char(inst);
	}

	size_t readBytes(char* buffer, size_t length) override
	{
		return tud_cdc_n_read(inst, buffer, length);
	}

	int peek() override
	{
		uint8_t c;
		return tud_cdc_n_peek(inst, &c) ? c : -1;
	}

	void clear(SerialMode mode = SERIAL_FULL) override
	{
		if(mode != SerialMode::TxOnly) {
			tud_cdc_n_read_flush(inst);
		}
		if(mode != SerialMode::RxOnly) {
			tud_cdc_n_write_clear(inst);
		}
	}

	void flush() override
	{
		tud_cdc_n_write_flush(inst);
	}

	using Stream::write;

	size_t write(const uint8_t* buffer, size_t size) override;
};

} // namespace USB::CDC
