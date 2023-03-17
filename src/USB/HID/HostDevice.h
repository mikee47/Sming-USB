/****
 * HID/HostDevice.h
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

namespace USB::HID
{
struct Report : public DescriptorList {
	unsigned parse(tuh_hid_report_info_t report_info_arr[], unsigned arr_count) const;
};

class HostDevice : public HostInterface
{
public:
	using ReportReceived = Delegate<void(const Report& report)>;

	using HostInterface::HostInterface;

	bool requestReport()
	{
		return tuh_hid_receive_report(inst.dev_addr, inst.idx);
	}

	void onReport(ReportReceived callback)
	{
		reportReceivedCallback = callback;
	}

	void reportReceived(const Report& report)
	{
		if(reportReceivedCallback) {
			reportReceivedCallback(report);
		}
	}

private:
	ReportReceived reportReceivedCallback;
};

/**
 * @brief Application callback to notify connection of a new device
 * @param inst TinyUSB device instance
 * @param report HID report descriptors for this interface
 * @retval HostDevice* Application returns pointer to implementation, or nullptr to ignore this device
 */
using MountCallback = Delegate<HostDevice*(const HostInterface::Instance& inst, const Report& report)>;

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

} // namespace USB::HID
