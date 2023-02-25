#pragma once

#include "../Interface.h"

namespace USB::HID
{
class Device : public Interface
{
public:
	using ReportComplete = Delegate<void()>;

	using Interface::Interface;

	bool sendReport(uint8_t report_id, void const* report, uint16_t len, ReportComplete callback);

	uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
	void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
	void report_complete();

private:
	ReportComplete reportCompleteCallback;
};

} // namespace USB::HID
