/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <SmingCore.h>
#include <USB.h>
#include "usb_descriptors.h"

#ifdef ARCH_RP2040
#include <pico.h>
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif
#endif

namespace
{
// LED blink interval (in milliseconds) depends on state
enum {
	BLINK_NOT_MOUNTED = 250,
	BLINK_MOUNTED = 1000,
	BLINK_SUSPENDED = 2500,
};

SimpleTimer taskTimer;
SimpleTimer ledTimer;
bool ledState;

#define FUNC() debug_i("%s()", __FUNCTION__);
#define FUNC_FMT(fmt, ...) debug_i("%s(" fmt ")", __FUNCTION__, __VA_ARGS__);

#ifdef PICO_DEFAULT_LED_PIN
#define LED_PIN PICO_DEFAULT_LED_PIN
#else
#define LED_PIN 2 // GPIO2
#endif

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

void sendHidReport(uint8_t report_id, uint32_t btn)
{
	if(!tud_hid_ready()) {
		return;
	}

	switch(report_id) {
	case REPORT_ID_KEYBOARD: {
		// avoid sending multiple consecutive zero reports for keyboard
		static bool has_keyboard_key{false};

		if(btn) {
			uint8_t keycode[6]{
				HID_KEY_A,
			};

			tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
			has_keyboard_key = true;
		} else if(has_keyboard_key) {
			// key previously pressed, send empty report
			tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, nullptr);
			has_keyboard_key = false;
		}
		break;
	}

	case REPORT_ID_MOUSE: {
		const int8_t delta{5};

		// no button, right + down, no scroll, no pan
		tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta, delta, 0, 0);
		break;
	}

	case REPORT_ID_CONSUMER_CONTROL: {
		// avoid sending multiple consecutive zero reports
		static bool has_consumer_key{false};

		if(btn) {
			// volume down
			uint16_t volume_down{HID_USAGE_CONSUMER_VOLUME_DECREMENT};
			tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_down, 2);
			has_consumer_key = true;
		} else {
			// send empty key report (release key) if previously has key pressed
			uint16_t empty_key{0};
			if(has_consumer_key) {
				tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
				has_consumer_key = false;
			}
		}
		break;
	}

	case REPORT_ID_GAMEPAD: {
		// use to avoid send multiple consecutive zero report for keyboard
		static bool has_gamepad_key{false};

		hid_gamepad_report_t report{
			.x = 0,
			.y = 0,
			.z = 0,
			.rz = 0,
			.rx = 0,
			.ry = 0,
			.hat = 0,
			.buttons = 0,
		};

		if(btn) {
			report.hat = GAMEPAD_HAT_UP;
			report.buttons = GAMEPAD_BUTTON_A;
			tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
			has_gamepad_key = true;
		} else {
			report.hat = GAMEPAD_HAT_CENTERED;
			report.buttons = 0;
			if(has_gamepad_key) {
				tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
				has_gamepad_key = false;
			}
		}
		break;
	}

	default:
		break;
	}
}

bool readButton()
{
	return false;
}

void setLed(bool state)
{
#ifdef CYW43_WL_GPIO_LED_PIN
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state);
#else
	digitalWrite(LED_PIN, state);
#endif

	ledState = state;
}

void toggleLed()
{
	setLed(!ledState);
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hidTask()
{
	static unsigned blinkTimer;
	++blinkTimer;
	if(blinkTimer % 64 == 0) {
	}

	uint32_t const btn = readButton();

	// Remote wakeup
	if(tud_suspended() && btn) {
		// Wake up host if we are in suspend mode
		// and REMOTE_WAKEUP feature is enabled by host
		tud_remote_wakeup();
	} else {
		// Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
		sendHidReport(REPORT_ID_KEYBOARD, btn);
	}
}

} // namespace

// Invoked when device is mounted
void tud_mount_cb()
{
	FUNC()
	ledTimer.setIntervalMs<BLINK_MOUNTED>();
}

// Invoked when device is unmounted
void tud_umount_cb()
{
	FUNC()
	ledTimer.setIntervalMs<BLINK_NOT_MOUNTED>();
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
	FUNC()
	(void)remote_wakeup_en;
	ledTimer.setIntervalMs<BLINK_SUSPENDED>();
}

// Invoked when usb bus is resumed
void tud_resume_cb()
{
	FUNC()
	ledTimer.setIntervalMs<BLINK_MOUNTED>();
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint8_t len)
{
	FUNC()
	(void)instance;
	(void)len;

	uint8_t next_report_id = report[0] + 1;

	if(next_report_id < REPORT_ID_COUNT) {
		sendHidReport(next_report_id, readButton());
	}
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer,
							   uint16_t reqlen)
{
	FUNC()
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
	FUNC_FMT("instance %u, report_id %u, report_type %u, bufsize %u", instance, report_id, report_type, bufsize);
	debug_hex(INFO, "BUF", buffer, bufsize);
	(void)instance;

	if(report_type == HID_REPORT_TYPE_OUTPUT) {
		// Set keyboard LED e.g Capslock, Numlock etc...
		if(report_id == REPORT_ID_KEYBOARD) {
			// bufsize should be (at least) 1
			if(bufsize < 1) {
				return;
			}

			uint8_t const kbd_leds = buffer[0];

			if(kbd_leds & KEYBOARD_LED_CAPSLOCK) {
				// Capslock On: disable blink, turn led on
				ledTimer.stop();
				setLed(true);
			} else {
				// Caplocks Off: back to normal blink
				setLed(false);
				ledTimer.setIntervalMs<BLINK_MOUNTED>();
				ledTimer.start();
			}
		}
	}
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true);

	pinMode(LED_PIN, OUTPUT);

	delay(1000);
	Serial << _F("USB sample started") << endl;

	USB::begin();
	taskTimer.initializeMs<10>(hidTask).start();
	ledTimer.initializeMs<BLINK_NOT_MOUNTED>(toggleLed).start();
}
