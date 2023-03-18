/****
 * VENDOR/HostDevice.h
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
#include <debug_progmem.h>
#include <bitset>

namespace USB::VENDOR
{
/**
 * @brief Base class to use for custom devices
 */
class HostDevice : public HostInterface
{
public:
	/**
	 * @brief Device configuration received during mount procedure
	 */
	struct Config {
		uint16_t vid;		 ///< Vendor ID
		uint16_t pid;		 ///< Product ID
		DescriptorList list; ///< Interface descriptor list
	};

	/**
	 * @brief Structure passed to 'transferComplete' method
	 */
	struct Transfer {
		uint8_t dev_addr;
		uint8_t ep_addr;
		xfer_result_t result;
		uint32_t xferred_bytes;
	};

	using HostInterface::HostInterface;

	void end() override
	{
		ep_mask.reset();
	}

	/**
	 * @brief Set active configuration
	 *
	 * Implementations typically start communicating.
	 */
	virtual bool setConfig(uint8_t itf_num) = 0;

	/**
	 * @brief Called when a non-control USB transfer has completed
	 */
	virtual bool transferComplete(const Transfer& txfr) = 0;

	bool ownsEndpoint(uint8_t ep_addr);

protected:
	/**
	 * @brief Implementations should call this method during initialisation
	 *
	 * Typically called in a 'begin' method called from the application.
	 * Endpoints are released automatically when the device is disconnected.
	 */
	bool openEndpoint(const tusb_desc_endpoint_t& ep_desc);

private:
	std::bitset<32> ep_mask{};
};

/**
 * @brief Application callback to notify connection of a new device
 * @param inst TinyUSB device instance
 * @param cfg HostDevice configuration
 * @retval HostDevice* Application returns pointer to implementation, or nullptr to ignore this device
 */
using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst, const HostDevice::Config& cfg)>;

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

} // namespace USB::VENDOR
