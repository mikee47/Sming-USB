/****
 * DFU/Device.h
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

#include "../DeviceInterface.h"

namespace USB::DFU
{
/**
 * @brief Applications must implement this class and pass an instance to Device::begin().
 */
class Callbacks
{
public:
	using Alternate = DfuAlternateId;

	/**
	 * @brief Invoked right before tud_dfu_download_cb() (state=DFU_DNBUSY) or tud_dfu_manifest_cb() (state=DFU_MANIFEST)
	 *
	 * Application return timeout in milliseconds (bwPollTimeout) for the next download/manifest operation.
	 * During this period, USB host won't try to communicate with us.
	 */
	virtual uint32_t getTimeout(Alternate alt, dfu_state_t state) = 0;

	/**
	 * @brief Invoked when received DFU_DNLOAD (wLength>0) following by DFU_GETSTATUS (state=DFU_DNBUSY) requests.
	 *
	 * This callback could be returned before flashing op is complete (async).
	 * Once finished flashing, application must call `complete()`
	 */
	virtual void download(Alternate alt, uint32_t offset, const void* data, uint16_t length) = 0;

	/**
	 * @brief Invoked when download process is complete, received DFU_DNLOAD (wLength=0) following by DFU_GETSTATUS (state=Manifest)
	 *
	 * Application can do checksum, or actual flashing if buffered entire image previously.
	 * Once finished flashing, application must call `Device::finishFlashing()`
	 */
	virtual void manifest(Alternate alt) = 0;

	/**
	 * @brief Invoked when received DFU_UPLOAD request
	 * Application must populate data with up to length bytes and return the number of written bytes.
 	 */
	virtual uint16_t upload(Alternate alt, uint32_t offset, void* data, uint16_t length) = 0;

	/**
	 * @brief Invoked when the Host has terminated a download or upload transfer
	 */
	virtual void abort(Alternate alt) = 0;

	/**
	 * @brief Invoked when a DFU_DETACH request is received
	 */
	virtual void detach() = 0;
};

class Device : public DeviceInterface
{
public:
	Device(uint8_t inst, const char* name);

	static void begin(Callbacks& callbacks);

	/**
	 * @brief Applications call this method from download and manifest callbacks
	 *
	 * This mechanism supports use of asynchronous writes.
	 */
	static void complete(dfu_status_t status)
	{
		tud_dfu_finish_flashing(status);
	}
};

} // namespace USB::DFU
