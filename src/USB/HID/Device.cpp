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

#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_HID

namespace USB::HID
{
class InternalDevice : public Device
{
public:
	using Device::get_report;
	using Device::report_complete;
	using Device::set_report;
};

InternalDevice* getDevice(uint8_t inst)
{
	extern InternalDevice* devices[];
	return (inst < CFG_TUD_HID) ? devices[inst] : nullptr;
}

uint16_t Device::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
	return 0;
}

void Device::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
	char buf[32];
	m_snprintf(buf, sizeof(buf), "%s(%u, %u, %u)", __FUNCTION__, report_id, report_type, bufsize);
	m_printHex(buf, buffer, bufsize);
}

bool Device::sendReport(uint8_t report_id, void const* report, uint16_t len, ReportComplete callback)
{
	if(reportCompleteCallback) {
		return false;
	}
	reportCompleteCallback = callback;
	return tud_hid_n_report(inst, report_id, report, len);
}

void Device::report_complete()
{
	if(reportCompleteCallback) {
		auto callback = reportCompleteCallback;
		reportCompleteCallback = nullptr;
		callback();
	}
}

} // namespace USB::HID

using namespace USB::HID;

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer,
							   uint16_t reqlen)
{
	auto dev = getDevice(instance);
	return dev ? dev->get_report(report_id, report_type, buffer, reqlen) : 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer,
						   uint16_t bufsize)
{
	auto dev = getDevice(instance);
	if(dev) {
		dev->set_report(report_id, report_type, buffer, bufsize);
	}
}

// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)
// void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol)
// {
// 	(void)instance;
// 	(void)protocol;
// 	// nothing to do since we use the same compatible boot report for both Boot and Report mode.
// 	// TODO set a indicator for user
// }

// Invoked when received SET_IDLE request. return false will stall the request
// - Idle Rate = 0 : only send report if there is changes, i.e skip duplication
// - Idle Rate > 0 : skip duplication, but send at least 1 report every idle rate (in unit of 4 ms).
//bool tud_hid_set_idle_cb(uint8_t instance, uint8_t idle_rate)
// {
// }

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
	auto dev = getDevice(instance);
	if(dev) {
		dev->report_complete();
	}
}

#endif
