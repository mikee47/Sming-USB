#pragma once

namespace USB::HID
{
class Device
{
public:
	Device(uint8_t instance, const char* name)
	{
	}

	uint16_t getReport(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);

	void setReport(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
};

} // namespace USB::HID
