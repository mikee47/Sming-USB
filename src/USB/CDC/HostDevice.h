#pragma once

#include "../HostInterface.h"
#include "UsbSerial.h"

namespace USB::CDC
{
/**
 * @brief Implements CDC interface for a connected serial device
 */
class HostDevice : public HostInterface, public UsbSerial
{
public:
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
		return tuh_cdc_read_available(inst.idx);
	}

	bool isFinished() override
	{
		return !tuh_cdc_mounted(inst.idx);
	}

	int read() override
	{
		char c;
		return tuh_cdc_read(inst.idx, &c, 1) ? c : -1;
	}

	size_t readBytes(char* buffer, size_t length) override
	{
		return tuh_cdc_read(inst.idx, buffer, length);
	}

	int peek() override
	{
		uint8_t c;
		return tuh_cdc_peek(inst.idx, &c) ? c : -1;
	}

	void clear(SerialMode mode = SERIAL_FULL) override
	{
		if(mode != SerialMode::TxOnly) {
			tuh_cdc_read_clear(inst.idx);
		}
		if(mode != SerialMode::RxOnly) {
			tuh_cdc_write_clear(inst.idx);
		}
	}

	void flush() override
	{
		tuh_cdc_write_flush(inst.idx);
	}

	using Stream::write;

	size_t write(const uint8_t* buffer, size_t size) override;
};

using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst)>;
using UnmountCallback = Delegate<void(HostDevice& dev)>;

void onMount(MountCallback callback);
void onUnmount(UnmountCallback callback);

} // namespace USB::CDC
