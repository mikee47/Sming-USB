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

namespace USB::VENDOR
{
bool Xbox::begin(const Instance& inst, const Config& cfg)
{
	if(cfg.vid != 0x045e || cfg.pid != 0x028e) {
		debug_d("[XBOX] Not an xbox 360 controller");
		return false;
	}

	if(!parseInterface(cfg.itf)) {
		return false;
	}

	debug_i("[XBOX] Found controller");

	if(daddr) {
		debug_w("[XBOX] Already initialised");
		return false;
	}

	daddr = inst.dev_addr;
	state = 0;

	return true;
}

bool Xbox::parseInterface(DescriptorEnum e)
{
	auto desc = e.next();
	if(!desc || desc->type != TUSB_DESC_INTERFACE) {
		return false;
	}
	auto itf = desc->as<tusb_desc_interface_t>();
	if(itf->bInterfaceSubClass != 93 || itf->bInterfaceProtocol != 1) {
		return false;
	}
	desc = e.next();
	if(!desc || desc->type != TUSB_DESC_CS_DEVICE) {
		return false;
	}
	desc = e.next();
	if(!desc || desc->type != TUSB_DESC_ENDPOINT) {
		return false;
	}
	ep_in = desc->as<tusb_desc_endpoint_t>()->bEndpointAddress;
	desc = e.next();
	if(!desc || desc->type != TUSB_DESC_ENDPOINT) {
		return false;
	}
	ep_out = desc->as<tusb_desc_endpoint_t>()->bEndpointAddress;
	if(ep_out & TUSB_DIR_IN_MASK) {
		std::swap(ep_in, ep_out);
	}

	debug_i("ep-in 0x%02x, ep-out 0x%02x", ep_in, ep_out);

	return true;
}

bool Xbox::setConfig(uint8_t itf_num)
{
	if(itf_num != 0) {
		debug_e("[XBOX] itf %u not supported", itf_num);
		return false;
	}

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

	return control(TUSB_REQ_RCPT_INTERFACE, 0x100, 20);
}

void Xbox::end()
{
	daddr = 0;
}

bool Xbox::control(tusb_request_recipient_t recipient, uint16_t value, uint16_t length)
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

	return tuh_control_xfer(&xfer);
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

bool Xbox::transferComplete(const Transfer& txfr)
{
	debug_hex(DBG, "RX", buffer, txfr.xferred_bytes);
	process_packet();
	read();
	return true;
}

bool Xbox::read()
{
	TU_VERIFY(usbh_edpt_claim(daddr, ep_in));

	if(!usbh_edpt_xfer(daddr, ep_in, buffer, bufSize)) {
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

const char* Xbox::getInputName(Xbox::Input input)
{
	switch(input) {
#define XX(tag, ...)                                                                                                   \
	case Xbox::Input::tag:                                                                                             \
		return #tag;
		XBOX360_INPUT_MAP(XX)
#undef XX
	default:
		return "?";
	}
}

void Xbox::process_packet()
{
	InputData data alignas(16);
	memcpy(&data, &buffer[2], sizeof(data));

	auto b = reinterpret_cast<const uint8_t*>(&data);
	uint16_t btn1 = b[0] | (b[1] << 8);
	b = reinterpret_cast<const uint8_t*>(&inputData);
	uint16_t btn2 = b[0] | (b[1] << 8);
	InputMask changed = btn1 ^ btn2;
	changed[Input::trig_left] = !compare(data.trig_left, inputData.trig_left);
	changed[Input::trig_right] = !compare(data.trig_right, inputData.trig_right);
	changed[Input::stick_left_x] = !compare(data.stick_left_x, inputData.stick_left_x);
	changed[Input::stick_left_y] = !compare(data.stick_left_y, inputData.stick_left_y);
	changed[Input::stick_right_x] = !compare(data.stick_right_x, inputData.stick_right_x);
	changed[Input::stick_right_y] = !compare(data.stick_right_y, inputData.stick_right_y);

	inputData = data;

	if(inputChangeCallback) {
		inputChangeCallback(changed);
	}
}

bool Xbox::output(const void* data, uint8_t length)
{
	TU_VERIFY(usbh_edpt_claim(daddr, ep_out));

	memcpy(output_buffer, data, length);
	if(!usbh_edpt_xfer(daddr, ep_out, output_buffer, length)) {
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

} // namespace USB::VENDOR
