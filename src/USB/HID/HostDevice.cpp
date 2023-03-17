/****
 * HID/HostDevice.cpp
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

#if CFG_TUH_ENABLED && CFG_TUH_HID

namespace USB::HID
{
MountCallback mountCallback;
UnmountCallback unmountCallback;
HostDevice* host_devices[CFG_TUH_HID];

// Report Item Types
#define REPORT_ITEM_TYPE_MAP(XX)                                                                                       \
	XX(MAIN, 0)                                                                                                        \
	XX(GLOBAL, 1)                                                                                                      \
	XX(LOCAL, 2)

const char* getReportItemTypeName(uint8_t type)
{
	switch(type) {
#define XX(name, value)                                                                                                \
	case value:                                                                                                        \
		return #name;
		REPORT_ITEM_TYPE_MAP(XX)
#undef XX
	default:
		return "?";
	}
}

// Report Item Main group
#define REPORT_ITEM_MAIN_GROUP_MAP(XX)                                                                                 \
	XX(INPUT, 8)                                                                                                       \
	XX(OUTPUT, 9)                                                                                                      \
	XX(COLLECTION, 10)                                                                                                 \
	XX(FEATURE, 11)                                                                                                    \
	XX(COLLECTION_END, 12)

const char* getReportItemMainGroupName(uint8_t tag)
{
	switch(tag) {
#define XX(name, value)                                                                                                \
	case value:                                                                                                        \
		return #name;
		REPORT_ITEM_MAIN_GROUP_MAP(XX)
#undef XX
	default:
		return "?";
	}
}

// Report Item Global group
#define REPORT_ITEM_GLOBAL_GROUP_MAP(XX)                                                                               \
	XX(USAGE_PAGE, 0)                                                                                                  \
	XX(LOGICAL_MIN, 1)                                                                                                 \
	XX(LOGICAL_MAX, 2)                                                                                                 \
	XX(PHYSICAL_MIN, 3)                                                                                                \
	XX(PHYSICAL_MAX, 4)                                                                                                \
	XX(UNIT_EXPONENT, 5)                                                                                               \
	XX(UNIT, 6)                                                                                                        \
	XX(REPORT_SIZE, 7)                                                                                                 \
	XX(REPORT_ID, 8)                                                                                                   \
	XX(REPORT_COUNT, 9)                                                                                                \
	XX(PUSH, 10)                                                                                                       \
	XX(POP, 11)

const char* getReportItemGlobalGroupName(uint8_t tag)
{
	switch(tag) {
#define XX(name, value)                                                                                                \
	case value:                                                                                                        \
		return #name;
		REPORT_ITEM_GLOBAL_GROUP_MAP(XX)
#undef XX
	default:
		return "?";
	}
}

// Report Item Local group
#define REPORT_ITEM_LOCAL_GROUP_MAP(XX)                                                                                \
	XX(USAGE, 0)                                                                                                       \
	XX(USAGE_MIN, 1)                                                                                                   \
	XX(USAGE_MAX, 2)                                                                                                   \
	XX(DESIGNATOR_INDEX, 3)                                                                                            \
	XX(DESIGNATOR_MIN, 4)                                                                                              \
	XX(DESIGNATOR_MAX, 5)                                                                                              \
	XX(RESERVED, 6)                                                                                                    \
	XX(STRING_INDEX, 7)                                                                                                \
	XX(STRING_MIN, 8)                                                                                                  \
	XX(STRING_MAX, 9)                                                                                                  \
	XX(DELIMITER, 10)

const char* getReportItemLocalGroupName(uint8_t tag)
{
	switch(tag) {
#define XX(name, value)                                                                                                \
	case value:                                                                                                        \
		return #name;
		REPORT_ITEM_LOCAL_GROUP_MAP(XX)
#undef XX
	default:
		return "?";
	}
}

const char* getReportItemTagName(uint8_t type, uint8_t tag)
{
	switch(type) {
	case RI_TYPE_MAIN:
		return getReportItemMainGroupName(tag);
	case RI_TYPE_GLOBAL:
		return getReportItemGlobalGroupName(tag);
	case RI_TYPE_LOCAL:
		return getReportItemLocalGroupName(tag);
	default:
		return "?";
	}
}

/// HID Usage Table - Table 1: Usage Page Summary
#define HID_USAGE_PAGE_MAP(XX)                                                                                         \
	XX(DESKTOP, 0x01)                                                                                                  \
	XX(SIMULATE, 0x02)                                                                                                 \
	XX(VIRTUAL_REALITY, 0x03)                                                                                          \
	XX(SPORT, 0x04)                                                                                                    \
	XX(GAME, 0x05)                                                                                                     \
	XX(GENERIC_DEVICE, 0x06)                                                                                           \
	XX(KEYBOARD, 0x07)                                                                                                 \
	XX(LED, 0x08)                                                                                                      \
	XX(BUTTON, 0x09)                                                                                                   \
	XX(ORDINAL, 0x0a)                                                                                                  \
	XX(TELEPHONY, 0x0b)                                                                                                \
	XX(CONSUMER, 0x0c)                                                                                                 \
	XX(DIGITIZER, 0x0d)                                                                                                \
	XX(PID, 0x0f)                                                                                                      \
	XX(UNICODE, 0x10)                                                                                                  \
	XX(ALPHA_DISPLAY, 0x14)                                                                                            \
	XX(MEDICAL, 0x40)                                                                                                  \
	XX(MONITOR, 0x80)                                                                                                  \
	XX(POWER, 0x84)                                                                                                    \
	XX(BARCODE_SCANNER, 0x8c)                                                                                          \
	XX(SCALE, 0x8d)                                                                                                    \
	XX(MSR, 0x8e)                                                                                                      \
	XX(CAMERA, 0x90)                                                                                                   \
	XX(ARCADE, 0x91)                                                                                                   \
	XX(FIDO, 0xF1D0)                                                                                                   \
	XX(VENDOR, 0xFF00)

const char* getHidUsagePageName(unsigned page)
{
	switch(page) {
#define XX(name, value)                                                                                                \
	case value:                                                                                                        \
		return #name;
		HID_USAGE_PAGE_MAP(XX)
#undef XX
	default:
		return "?";
	}
}

/// HID Usage Table - Table 6: Generic Desktop Page
#define HID_USAGE_DESKTOP_MAP(XX)                                                                                      \
	XX(POINTER, 0x01)                                                                                                  \
	XX(MOUSE, 0x02)                                                                                                    \
	XX(JOYSTICK, 0x04)                                                                                                 \
	XX(GAMEPAD, 0x05)                                                                                                  \
	XX(KEYBOARD, 0x06)                                                                                                 \
	XX(KEYPAD, 0x07)                                                                                                   \
	XX(MULTI_AXIS_CONTROLLER, 0x08)                                                                                    \
	XX(TABLET_PC_SYSTEM, 0x09)                                                                                         \
	XX(X, 0x30)                                                                                                        \
	XX(Y, 0x31)                                                                                                        \
	XX(Z, 0x32)                                                                                                        \
	XX(RX, 0x33)                                                                                                       \
	XX(RY, 0x34)                                                                                                       \
	XX(RZ, 0x35)                                                                                                       \
	XX(SLIDER, 0x36)                                                                                                   \
	XX(DIAL, 0x37)                                                                                                     \
	XX(WHEEL, 0x38)                                                                                                    \
	XX(HAT_SWITCH, 0x39)                                                                                               \
	XX(COUNTED_BUFFER, 0x3a)                                                                                           \
	XX(BYTE_COUNT, 0x3b)                                                                                               \
	XX(MOTION_WAKEUP, 0x3c)                                                                                            \
	XX(START, 0x3d)                                                                                                    \
	XX(SELECT, 0x3e)                                                                                                   \
	XX(VX, 0x40)                                                                                                       \
	XX(VY, 0x41)                                                                                                       \
	XX(VZ, 0x42)                                                                                                       \
	XX(VBRX, 0x43)                                                                                                     \
	XX(VBRY, 0x44)                                                                                                     \
	XX(VBRZ, 0x45)                                                                                                     \
	XX(VNO, 0x46)                                                                                                      \
	XX(FEATURE_NOTIFICATION, 0x47)                                                                                     \
	XX(RESOLUTION_MULTIPLIER, 0x48)                                                                                    \
	XX(SYSTEM_CONTROL, 0x80)                                                                                           \
	XX(SYSTEM_POWER_DOWN, 0x81)                                                                                        \
	XX(SYSTEM_SLEEP, 0x82)                                                                                             \
	XX(SYSTEM_WAKE_UP, 0x83)                                                                                           \
	XX(SYSTEM_CONTEXT_MENU, 0x84)                                                                                      \
	XX(SYSTEM_MAIN_MENU, 0x85)                                                                                         \
	XX(SYSTEM_APP_MENU, 0x86)                                                                                          \
	XX(SYSTEM_MENU_HELP, 0x87)                                                                                         \
	XX(SYSTEM_MENU_EXIT, 0x88)                                                                                         \
	XX(SYSTEM_MENU_SELECT, 0x89)                                                                                       \
	XX(SYSTEM_MENU_RIGHT, 0x8A)                                                                                        \
	XX(SYSTEM_MENU_LEFT, 0x8B)                                                                                         \
	XX(SYSTEM_MENU_UP, 0x8C)                                                                                           \
	XX(SYSTEM_MENU_DOWN, 0x8D)                                                                                         \
	XX(SYSTEM_COLD_RESTART, 0x8E)                                                                                      \
	XX(SYSTEM_WARM_RESTART, 0x8F)                                                                                      \
	XX(DPAD_UP, 0x90)                                                                                                  \
	XX(DPAD_DOWN, 0x91)                                                                                                \
	XX(DPAD_RIGHT, 0x92)                                                                                               \
	XX(DPAD_LEFT, 0x93)                                                                                                \
	XX(SYSTEM_DOCK, 0xA0)                                                                                              \
	XX(SYSTEM_UNDOCK, 0xA1)                                                                                            \
	XX(SYSTEM_SETUP, 0xA2)                                                                                             \
	XX(SYSTEM_BREAK, 0xA3)                                                                                             \
	XX(SYSTEM_DEBUGGER_BREAK, 0xA4)                                                                                    \
	XX(APPLICATION_BREAK, 0xA5)                                                                                        \
	XX(APPLICATION_DEBUGGER_BREAK, 0xA6)                                                                               \
	XX(SYSTEM_SPEAKER_MUTE, 0xA7)                                                                                      \
	XX(SYSTEM_HIBERNATE, 0xA8)                                                                                         \
	XX(SYSTEM_DISPLAY_INVERT, 0xB0)                                                                                    \
	XX(SYSTEM_DISPLAY_INTERNAL, 0xB1)                                                                                  \
	XX(SYSTEM_DISPLAY_EXTERNAL, 0xB2)                                                                                  \
	XX(SYSTEM_DISPLAY_BOTH, 0xB3)                                                                                      \
	XX(SYSTEM_DISPLAY_DUAL, 0xB4)                                                                                      \
	XX(SYSTEM_DISPLAY_TOGGLE_INT_EXT, 0xB5)                                                                            \
	XX(SYSTEM_DISPLAY_SWAP_PRIMARY_SECONDARY, 0xB6)                                                                    \
	XX(SYSTEM_DISPLAY_LCD_AUTOSCALE, 0xB7)

const char* getHidUsageDesktopName(uint8_t usage)
{
	switch(usage) {
#define XX(name, value)                                                                                                \
	case value:                                                                                                        \
		return #name;
		HID_USAGE_DESKTOP_MAP(XX)
#undef XX
	default:
		return "?";
	}
}

/// HID Usage Table: Consumer Page (0x0C)
/// Only contains controls that supported by Windows (whole list is too long)
#define HID_USAGE_CONSUMER_MAP(XX)                                                                                     \
	XX(CONTROL, 0x0001)                                                                                                \
	XX(POWER, 0x0030)                                                                                                  \
	XX(RESET, 0x0031)                                                                                                  \
	XX(SLEEP, 0x0032)                                                                                                  \
	XX(BRIGHTNESS_INCREMENT, 0x006F)                                                                                   \
	XX(BRIGHTNESS_DECREMENT, 0x0070)                                                                                   \
	XX(WIRELESS_RADIO_CONTROLS, 0x000C)                                                                                \
	XX(WIRELESS_RADIO_BUTTONS, 0x00C6)                                                                                 \
	XX(WIRELESS_RADIO_LED, 0x00C7)                                                                                     \
	XX(WIRELESS_RADIO_SLIDER_SWITCH, 0x00C8)                                                                           \
	XX(PLAY_PAUSE, 0x00CD)                                                                                             \
	XX(SCAN_NEXT, 0x00B5)                                                                                              \
	XX(SCAN_PREVIOUS, 0x00B6)                                                                                          \
	XX(STOP, 0x00B7)                                                                                                   \
	XX(VOLUME, 0x00E0)                                                                                                 \
	XX(MUTE, 0x00E2)                                                                                                   \
	XX(BASS, 0x00E3)                                                                                                   \
	XX(TREBLE, 0x00E4)                                                                                                 \
	XX(BASS_BOOST, 0x00E5)                                                                                             \
	XX(VOLUME_INCREMENT, 0x00E9)                                                                                       \
	XX(VOLUME_DECREMENT, 0x00EA)                                                                                       \
	XX(BASS_INCREMENT, 0x0152)                                                                                         \
	XX(BASS_DECREMENT, 0x0153)                                                                                         \
	XX(TREBLE_INCREMENT, 0x0154)                                                                                       \
	XX(TREBLE_DECREMENT, 0x0155)                                                                                       \
	XX(AL_CONSUMER_CONTROL_CONFIGURATION, 0x0183)                                                                      \
	XX(AL_EMAIL_READER, 0x018A)                                                                                        \
	XX(AL_CALCULATOR, 0x0192)                                                                                          \
	XX(AL_LOCAL_BROWSER, 0x0194)                                                                                       \
	XX(AC_SEARCH, 0x0221)                                                                                              \
	XX(AC_HOME, 0x0223)                                                                                                \
	XX(AC_BACK, 0x0224)                                                                                                \
	XX(AC_FORWARD, 0x0225)                                                                                             \
	XX(AC_STOP, 0x0226)                                                                                                \
	XX(AC_REFRESH, 0x0227)                                                                                             \
	XX(AC_BOOKMARKS, 0x022A)                                                                                           \
	XX(AC_PAN, 0x0238)

const char* getHidUsageConsumerName(unsigned usage)
{
	switch(usage) {
#define XX(name, value)                                                                                                \
	case value:                                                                                                        \
		return #name;
		HID_USAGE_CONSUMER_MAP(XX)
#undef XX
	default:
		return "?";
	}
}

const char* getHidUsageName(unsigned page, unsigned usage)
{
	switch(page) {
	case HID_USAGE_PAGE_DESKTOP:
		return getHidUsageDesktopName(usage);
	case HID_USAGE_PAGE_CONSUMER:
		return getHidUsageConsumerName(usage);
	case HID_USAGE_PAGE_SIMULATE:
	case HID_USAGE_PAGE_VIRTUAL_REALITY:
	case HID_USAGE_PAGE_SPORT:
	case HID_USAGE_PAGE_GAME:
	case HID_USAGE_PAGE_GENERIC_DEVICE:
	case HID_USAGE_PAGE_KEYBOARD:
	case HID_USAGE_PAGE_LED:
	case HID_USAGE_PAGE_BUTTON:
	case HID_USAGE_PAGE_ORDINAL:
	case HID_USAGE_PAGE_TELEPHONY:
	case HID_USAGE_PAGE_DIGITIZER:
	case HID_USAGE_PAGE_PID:
	case HID_USAGE_PAGE_UNICODE:
	case HID_USAGE_PAGE_ALPHA_DISPLAY:
	case HID_USAGE_PAGE_MEDICAL:
	case HID_USAGE_PAGE_MONITOR:
	case HID_USAGE_PAGE_POWER:
	case HID_USAGE_PAGE_BARCODE_SCANNER:
	case HID_USAGE_PAGE_SCALE:
	case HID_USAGE_PAGE_MSR:
	case HID_USAGE_PAGE_CAMERA:
	case HID_USAGE_PAGE_ARCADE:
	case HID_USAGE_PAGE_FIDO:
	case HID_USAGE_PAGE_VENDOR:
	default:
		return "?";
	}
}

void onMount(MountCallback callback)
{
	mountCallback = callback;
}

void onUnmount(UnmountCallback callback)
{
	unmountCallback = callback;
}

HostDevice* getDevice(HostDevice::Instance inst)
{
	for(auto dev : host_devices) {
		if(dev && *dev == inst) {
			return dev;
		}
	}

	return nullptr;
}

unsigned Report::parse(tuh_hid_report_info_t report_info_arr[], unsigned arr_count) const
{
	// Report Item 6.2.2.2 USB HID 1.11
	union Header {
		uint8_t byte;
		struct {
			uint8_t size : 2;
			uint8_t type : 2;
			uint8_t tag : 4;
		};
	};

	memset(report_info_arr, 0, sizeof(report_info_arr[0]) * arr_count);

	unsigned report_num{0};
	unsigned ri_collection_depth{0};
	for(unsigned offset = 0; offset < length && report_num < arr_count;) {
		auto data = reinterpret_cast<const uint8_t*>(desc) + offset;
		auto& info = report_info_arr[report_num];
		const Header hdr{*data++};
		++offset;

		TU_LOG(3, "type = %u (%s), tag = %u (%s), size = %d, data = ", hdr.type, getReportItemTypeName(hdr.type),
			   hdr.tag, getReportItemTagName(hdr.type, hdr.tag), hdr.size);
		for(unsigned i = 0; i < hdr.size; ++i) {
			TU_LOG(3, "%02X ", data[i]);
		}
		TU_LOG(3, "\r\n");

		switch(hdr.type) {
		case RI_TYPE_MAIN:
			switch(hdr.tag) {
			case RI_MAIN_INPUT:
				break;
			case RI_MAIN_OUTPUT:
				break;
			case RI_MAIN_FEATURE:
				break;

			case RI_MAIN_COLLECTION:
				++ri_collection_depth;
				break;

			case RI_MAIN_COLLECTION_END:
				--ri_collection_depth;
				if(ri_collection_depth == 0) {
					++report_num;
				}
				break;

			default:
				break;
			}
			break;

		case RI_TYPE_GLOBAL:
			switch(hdr.tag) {
			case RI_GLOBAL_USAGE_PAGE: {
				uint32_t page{0};
				memcpy(&page, data, hdr.size);
				debug_i("Usage page: %u (%s)", page, getHidUsagePageName(page));
				// only take in account the "usage page" before REPORT ID
				if(ri_collection_depth == 0) {
					info.usage_page = page;
				}
				break;
			}

			case RI_GLOBAL_LOGICAL_MIN:
				break;
			case RI_GLOBAL_LOGICAL_MAX:
				break;
			case RI_GLOBAL_PHYSICAL_MIN:
				break;
			case RI_GLOBAL_PHYSICAL_MAX:
				break;

			case RI_GLOBAL_REPORT_ID:
				info.report_id = data[0];
				break;

			case RI_GLOBAL_REPORT_SIZE:
				//            ri_report_size = data[0];
				break;

			case RI_GLOBAL_REPORT_COUNT:
				//            ri_report_count = data[0];
				break;

			case RI_GLOBAL_UNIT_EXPONENT:
				break;
			case RI_GLOBAL_UNIT:
				break;
			case RI_GLOBAL_PUSH:
				break;
			case RI_GLOBAL_POP:
				break;

			default:
				break;
			}
			break;

		case RI_TYPE_LOCAL:
			switch(hdr.tag) {
			case RI_LOCAL_USAGE: {
				auto usage = data[0];
				debug_i("Usage %u (%s)", usage, getHidUsageName(info.usage_page, usage));
				// only take in account the "usage" before starting REPORT ID
				if(ri_collection_depth == 0) {
					info.usage = usage;
				}
				break;
			}

			case RI_LOCAL_USAGE_MIN:
				break;
			case RI_LOCAL_USAGE_MAX:
				break;
			case RI_LOCAL_DESIGNATOR_INDEX:
				break;
			case RI_LOCAL_DESIGNATOR_MIN:
				break;
			case RI_LOCAL_DESIGNATOR_MAX:
				break;
			case RI_LOCAL_STRING_INDEX:
				break;
			case RI_LOCAL_STRING_MIN:
				break;
			case RI_LOCAL_STRING_MAX:
				break;
			case RI_LOCAL_DELIMITER:
				break;
			default:
				break;
			}
			break;

		// error
		default:
			break;
		}

		offset += hdr.size;
	}

	return report_num;
}

} // namespace USB::HID

using namespace USB::HID;

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report_desc, uint16_t desc_len)
{
	if(!mountCallback) {
		return;
	}

	HostDevice::Instance inst{dev_addr, instance, "hid"};
	Report report{reinterpret_cast<const USB::Descriptor*>(report_desc), desc_len};
	host_devices[instance] = mountCallback(inst, report);
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
	auto dev = getDevice({dev_addr, instance});
	if(!dev) {
		return;
	}

	dev->end();
	if(unmountCallback) {
		unmountCallback(*dev);
	}
}

// Invoked when received report from device via interrupt endpoint
// Note: if there is report ID (composite), it is 1st byte of report
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
	debug_i("%s(%u/%u, len %u)", __FUNCTION__, dev_addr, instance, len);
	auto dev = getDevice({dev_addr, instance});
	if(dev) {
		dev->reportReceived(Report{reinterpret_cast<const USB::Descriptor*>(report), len});
	}
}

// Invoked when sent report to device successfully via interrupt endpoint
void tuh_hid_report_sent_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
	debug_i("%s(%u/%u, len %u)", __FUNCTION__, dev_addr, instance, len);
}

// Invoked when Sent Report to device via either control endpoint
// len = 0 indicate there is error in the transfer e.g stalled response
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type,
									uint16_t len)
{
	debug_i("%s(%u/%u, id %u, type %u, len %u)", __FUNCTION__, dev_addr, instance, report_id, report_type, len);
}

// Invoked when Set Protocol request is complete
void tuh_hid_set_protocol_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t protocol)
{
	debug_i("%s(%u/%u, protocol %u)", __FUNCTION__, dev_addr, instance, protocol);
}

#endif
