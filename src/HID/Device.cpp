#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_HID

namespace USB::HID
{
Device* getDevice(uint8_t inst)
{
	extern Device* devices[];
	return (inst < CFG_TUD_HID) ? devices[inst] : nullptr;
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
	return dev ? dev->getReport(report_id, report_type, buffer, reqlen) : 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer,
						   uint16_t bufsize)
{
	auto dev = getDevice(instance);
	if(dev) {
		dev->setReport(report_id, report_type, buffer, bufsize);
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
//void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
// {
// }

namespace USB::HID
{
uint16_t Device::getReport(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
	return 0;
}

void Device::setReport(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

} // namespace USB::HID

#endif
