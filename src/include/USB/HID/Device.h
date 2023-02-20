#pragma once

#include <usb_descriptors.h>
#include <Delegate.h>

namespace USB::HID
{
class Device
{
public:
	using ReportComplete = Delegate<void()>;

	Device(uint8_t instance, const char* name) : inst(instance)
	{
	}

	bool sendReport(uint8_t report_id, void const* report, uint16_t len, ReportComplete callback);

	uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
	void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
	void report_complete();

private:
	uint8_t inst;
	ReportComplete reportCompleteCallback;
};

} // namespace USB::HID
