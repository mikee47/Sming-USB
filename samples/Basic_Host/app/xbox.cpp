/*
 * Code based on the linux xpad driver.
 * drivers/input/joystick/xpad.c
 *
 * The purpose is to explore how best to construct custom drivers for tinyusb using C++.
 * To keep things simple, supports only the XBOX 360 controller.
 * This is not a full xpad implementation!
 */

#include "xbox.h"
#include <host/usbh_classdriver.h>

bool Xbox::probe(uint8_t dev_addr)
{
	uint16_t vid{};
	uint16_t pid{};
	tuh_vid_pid_get(dev_addr, &vid, &pid);
	return vid == 0x045e && pid == 0x028e;
}

bool Xbox::init(uint8_t dev_addr)
{
	if(!probe(dev_addr)) {
		debug_w("Not an xbox 360 controller");
		return false;
	}

	daddr = dev_addr;
	state = 0;

	tusb_desc_endpoint_t ep_desc = {
		.bLength = sizeof(tusb_desc_endpoint_t),
		.bDescriptorType = TUSB_DESC_ENDPOINT,
		.bEndpointAddress = ep_in,
		.bmAttributes =
			{
				.xfer = TUSB_XFER_INTERRUPT,
				.sync = 0,
				.usage = 0,
			},
		.wMaxPacketSize = 64,
		.bInterval = 0,
	};
	TU_ASSERT(tuh_edpt_open(daddr, &ep_desc));
	ep_desc.bEndpointAddress = ep_out;
	TU_ASSERT(tuh_edpt_open(daddr, &ep_desc));

	control(TUSB_REQ_RCPT_INTERFACE, 0x100, 20);

	return true;
}

void Xbox::control(tusb_request_recipient_t recipient, uint16_t value, uint16_t length)
{
	static uint8_t buffer[20]{};
	const tusb_control_request_t request = {
		.bmRequestType_bit = {.recipient = recipient, .type = TUSB_REQ_TYPE_VENDOR, .direction = TUSB_DIR_IN},
		.bRequest = 1,
		.wValue = tu_htole16(value),
		.wIndex = tu_htole16(0),
		.wLength = tu_htole16(length),
	};

	tuh_xfer_t xfer = {
		.daddr = daddr,
		.ep_addr = 0,
		.setup = &request,
		.buffer = buffer,
		.complete_cb = static_control_cb,
		.user_data = uintptr_t(this),
	};

	tuh_control_xfer(&xfer);
}

void Xbox::control_cb(tuh_xfer_t* xfer)
{
	debug_i("%s(%u)", __FUNCTION__, xfer->user_data);
	switch(state++) {
	case 0:
		control(TUSB_REQ_RCPT_INTERFACE, 0, 8);
		break;
	case 1:
		control(TUSB_REQ_RCPT_DEVICE, 0, 4);
		break;
	case 2:
		setled(LedCommand::rotate2);
		read();
	}
}

bool Xbox::read()
{
	TU_VERIFY(usbh_edpt_claim(daddr, ep_in));

	auto callback = [](tuh_xfer_t* xfer) {
		auto self = reinterpret_cast<Xbox*>(xfer->user_data);
		debug_hex(DBG, "RX", self->buffer, xfer->actual_len);
		self->process_packet();
		self->read();
	};

	if(!usbh_edpt_xfer_with_callback(daddr, ep_in, buffer, bufSize, callback, uintptr_t(this))) {
		usbh_edpt_release(daddr, ep_in);
		return false;
	}

	return true;
}

template <typename T> bool compare(T a, T b)
{
	return a == b;
}

template <> bool compare(int16_t a, int16_t b)
{
	return abs(a - b) < 256;
}

void Xbox::process_packet()
{
	auto data = buffer;

	Inputs inputs{
		.dpad_up = !!(data[2] & 0x01),
		.dpad_down = !!(data[2] & 0x02),
		.dpad_left = !!(data[2] & 0x04),
		.dpad_right = !!(data[2] & 0x08),

		/* start/back buttons */
		.btn_start = !!(data[2] & BIT(4)),
		.btn_back = !!(data[2] & BIT(5)),

		/* stick press left/right */
		.btn_stick_left = bool(data[2] & BIT(6)),
		.btn_stick_right = bool(data[2] & BIT(7)),

		/* buttons A,B,X,Y,TL,TR and MODE */
		.btn_a = !!(data[3] & BIT(4)),
		.btn_b = !!(data[3] & BIT(5)),
		.btn_x = !!(data[3] & BIT(6)),
		.btn_y = !!(data[3] & BIT(7)),
		.btn_trig_left = !!(data[3] & BIT(0)),
		.btn_trig_right = !!(data[3] & BIT(1)),
		.btn_mode = !!(data[3] & BIT(2)),

		/* triggers left/right */
		.trig_left = data[4],
		.trig_right = data[5],

		/* left stick */
		.stick_left_x = int16_t(data[6] | (data[7] << 8)),
		.stick_left_y = int16_t(~(data[8] | (data[9] << 8))),

		/* right stick */
		.stick_right_x = int16_t(data[10] | (data[11] << 8)),
		.stick_right_y = int16_t(~(data[12] | (data[13] << 8))),
	};

#define XX(tag, ...)                                                                                                   \
	if(!compare(inputs.tag, prevInputs.tag)) {                                                                         \
		Serial << #tag ": " << inputs.tag << endl;                                                                     \
	}
	XBOX360_INPUT_MAP(XX)
#undef XX
	prevInputs = inputs;
}

bool Xbox::output(const void* data, uint8_t length)
{
	TU_VERIFY(usbh_edpt_claim(daddr, ep_out));

	auto callback = [](tuh_xfer_t* xfer) {};

	memcpy(output_buffer, data, length);
	if(!usbh_edpt_xfer_with_callback(daddr, ep_out, output_buffer, length, callback, uintptr_t(this))) {
		usbh_edpt_release(daddr, ep_out);
		return false;
	}

	return true;
}

bool Xbox::setled(LedCommand cmd)
{
	const uint8_t data[] = {
		0x01,
		0x03,
		uint8_t(cmd),
	};
	return output(data, ARRAY_SIZE(data));
}

bool Xbox::rumble(uint8_t strong, uint8_t weak)
{
	const uint8_t data[] = {
		0x00, 0x08, 0x00, strong, weak, 0x00, 0x00, 0x00,
	};

	return output(data, ARRAY_SIZE(data));
}
