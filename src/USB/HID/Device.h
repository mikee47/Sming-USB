/****
 * HID/Device.cpp
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

namespace USB::HID
{
class Device : public DeviceInterface
{
public:
	using ReportComplete = Delegate<void()>;

	using DeviceInterface::DeviceInterface;

	bool sendReport(uint8_t report_id, void const* report, uint16_t len, ReportComplete callback);

protected:
	uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
	void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
	void report_complete();

private:
	ReportComplete reportCompleteCallback;
};

} // namespace USB::HID
