/****
 * CDC/HostDevice.h
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

} // namespace USB::CDC
