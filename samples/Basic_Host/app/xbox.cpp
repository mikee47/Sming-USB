#include "xbox.h"
#include <host/usbh_classdriver.h>

bool Xbox::init(uint8_t dev_addr)
{
	daddr = dev_addr;

	const tusb_desc_endpoint_t ep_desc = {
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
		read();

		// case 2:
		//     tuh_descriptor_get_manufacturer_string(xfer->daddr, 0, info.manf, sizeof(info.manf), static_control_cb,
		//                                         uintptr_t(this));
		//     break;
		// case 3:
		// 	tuh_descriptor_get_product_string(xfer->daddr, 0, info.product, sizeof(info.product), transferCallback, 4);
		// 	break;
		// case 4:
		// 	tuh_descriptor_get_serial_string(xfer->daddr, 0, info.serial, sizeof(info.serial), transferCallback, 5);
		// 	break;
		// case 5:
		// 	m_printHex("MANF", info.manf, xfer->actual_len);
		// 	m_printHex("PROD", info.product, xfer->actual_len);
		// 	m_printHex("SER", info.serial, xfer->actual_len);
		// 	break;
	}
}

bool Xbox::read()
{
	TU_VERIFY(usbh_edpt_claim(daddr, ep_in));

	auto callback = [](tuh_xfer_t* xfer) {
		auto self = reinterpret_cast<Xbox*>(xfer->user_data);
		m_printHex("RX", self->buffer, xfer->actual_len);
		self->read();
	};

	if(!usbh_edpt_xfer_with_callback(daddr, ep_in, buffer, bufSize, callback, uintptr_t(this))) {
		usbh_edpt_release(daddr, ep_in);
		return false;
	}

	return true;
}

/*
 * set the LEDs on Xbox360 / Wireless Controllers
 * @param command
 *  0: off
 *  1: all blink, then previous setting
 *  2: 1/top-left blink, then on
 *  3: 2/top-right blink, then on
 *  4: 3/bottom-left blink, then on
 *  5: 4/bottom-right blink, then on
 *  6: 1/top-left on
 *  7: 2/top-right on
 *  8: 3/bottom-left on
 *  9: 4/bottom-right on
 * 10: rotate
 * 11: blink, based on previous setting
 * 12: slow blink, based on previous setting
 * 13: rotate with two lights
 * 14: persistent slow all blink
 * 15: blink once, then previous setting
 */
bool Xbox::setled(LedCommand cmd)
{
	// struct xpad_output_packet *packet = &xpad->out_packets[XPAD_OUT_LED_IDX];
	// unsigned long flags;

	// command %= 16;

	// packet->data[0] = 0x01;
	// packet->data[1] = 0x03;
	// packet->data[2] = command;
	// packet->len = 3;
	// packet->pending = true;

	// xpad_try_sending_next_out_packet(xpad);

	return false;
}
