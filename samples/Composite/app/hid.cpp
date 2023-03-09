#include "usbdev.h"

// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol)
{
	debug_i("%s(%u, %u)", __FUNCTION__, instance, protocol);
	(void)instance;
	(void)protocol;

	// nothing to do since we use the same compatible boot report for both Boot and Report mode.
	// TODO set a indicator for user
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
	debug_i("%s(%u, %u)", __FUNCTION__, instance, len);
	(void)instance;
	(void)report;
	(void)len;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer,
							   uint16_t reqlen)
{
	debug_i("%s(%u, %u, %u)", __FUNCTION__, instance, report_id, reqlen);
	// TODO not Implemented
	(void)instance;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)reqlen;

	return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer,
						   uint16_t bufsize)
{
	debug_i("%s(%u, %u, %u, %u)", __FUNCTION__, instance, report_id, report_type, bufsize);
	(void)report_id;
}
