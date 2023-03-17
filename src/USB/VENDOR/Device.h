#pragma once

#include "../CDC/UsbSerial.h"
#include "../DeviceInterface.h"

namespace USB::VENDOR
{
/**
 * @brief The TinyUSB vendor API is very much like a serial port.
 * Each instance corresponds to a bi-directional interface.
 */
class Device : public DeviceInterface, public USB::CDC::UsbSerial
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
		return tud_vendor_n_available(inst);
	}

	bool isFinished() override
	{
		return !tud_vendor_n_mounted(inst);
	}

	int read() override
	{
		char c;
		return tud_vendor_n_read(inst, &c, sizeof(c)) ? c : -1;
	}

	size_t readBytes(char* buffer, size_t length) override
	{
		return tud_vendor_n_read(inst, buffer, length);
	}

	int peek() override
	{
		uint8_t c;
		return tud_vendor_n_peek(inst, &c) ? c : -1;
	}

	void clear(SerialMode mode = SERIAL_FULL) override
	{
		if(mode != SerialMode::TxOnly) {
			tud_vendor_n_read_flush(inst);
		}
		if(mode != SerialMode::RxOnly) {
			tud_vendor_n_flush(inst); // There's no 'clear' API
		}
	}

	void flush() override
	{
		tud_vendor_n_flush(inst);
	}

	using Stream::write;

	size_t write(const uint8_t* buffer, size_t size) override;
};

} // namespace USB::VENDOR
