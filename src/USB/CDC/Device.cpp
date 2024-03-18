/****
 * CDC/UsbSerial.h
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

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_CDC

namespace USB::CDC
{
Device* getDevice(uint8_t inst)
{
	extern Device* devices[];
	return (inst < CFG_TUD_CDC) ? devices[inst] : nullptr;
}

Device::Device(uint8_t idx, const char* name) : DeviceInterface(idx, name), UsbSerial()
{
}

size_t Device::write(const uint8_t* buffer, size_t size)
{
	size_t written{0};
	while(size != 0) {
		size_t n = tud_cdc_n_write_available(inst);
		if(n == 0) {
			tud_cdc_n_write_flush(inst);
		} else {
			n = std::min(n, size);
			tud_cdc_n_write(inst, buffer, n);
			written += n;
			buffer += n;
			size -= n;
		}
		if(!bitRead(options, UART_OPT_TXWAIT)) {
			break;
		}
		tud_task_ext(0, true);
	}

	flush();

	return written;
}

} // namespace USB::CDC

using namespace USB::CDC;

// Invoked when received new data
void tud_cdc_rx_cb(uint8_t inst)
{
	auto dev = getDevice(inst);
	if(dev) {
		dev->handleEvent(Event::rx_data);
	}
}

// Invoked when received `wanted_char`
// void tud_cdc_rx_wanted_cb(uint8_t inst, char wanted_char)
// {
// 	debug_i("%s(%u, %u)", __FUNCTION__, inst, wanted_char);
// }

// Invoked when a TX is complete and therefore space becomes available in TX buffer
void tud_cdc_tx_complete_cb(uint8_t inst)
{
	auto dev = getDevice(inst);
	if(dev) {
		dev->handleEvent(Event::tx_done);
	}
}

// Invoked when line state DTR & RTS are changed via SET_CONTROL_LINE_STATE
void tud_cdc_line_state_cb(uint8_t inst, bool dtr, bool rts)
{
	debug_i("%s(%u, DTR %u, RTS %u)", __FUNCTION__, inst, dtr, rts);
}

// Invoked when line coding is change via SET_LINE_CODING
void tud_cdc_line_coding_cb(uint8_t inst, cdc_line_coding_t const* p_line_coding)
{
	debug_i("%s(%u, %u, %u-%u-%u)", __FUNCTION__, inst, p_line_coding->bit_rate, p_line_coding->data_bits,
			p_line_coding->parity, p_line_coding->stop_bits);
}

// Invoked when received send break
void tud_cdc_send_break_cb(uint8_t inst, uint16_t duration_ms)
{
	auto dev = getDevice(inst);
	if(dev) {
		dev->handleEvent(Event::line_break);
	}
}

#endif
