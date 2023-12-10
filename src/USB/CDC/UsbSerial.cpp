/****
 * CDC/UsbSerial.cpp
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

#if defined(ENABLE_USB_CLASSES)

#include "Platform/System.h"
#include <SimpleTimer.h>

namespace USB::CDC
{
UsbSerial::UsbSerial()
{
	flushTimer.initializeMs<50>(
		[](void* param) {
			auto self = static_cast<UsbSerial*>(param);
			self->flush();
		},
		this);
}

void UsbSerial::handleEvent(Event event)
{
	if(event == Event::line_break) {
		bitSet(status, eSERS_BreakDetected);
		return;
	}

	if(!eventMask) {
		System.queueCallback(
			[](void* param) {
				auto self = static_cast<UsbSerial*>(param);
				self->processEvents();
			},
			this);
	}

	eventMask += event;
}

void UsbSerial::processEvents()
{
	auto evt = eventMask;
	eventMask = 0;
	if(evt[Event::rx_data]) {
		if(receiveCallback) {
			receiveCallback(*this, peek(), available());
		}
	}

	if(evt[Event::tx_done]) {
		if(transmitCompleteCallback) {
			transmitCompleteCallback(*this);
		}
	}
}

void UsbSerial::systemDebugOutput(bool enabled)
{
	if(enabled) {
		m_setPuts([this](const char* data, size_t length) -> size_t { return write(data, length); });
	} else {
		m_setPuts(nullptr);
	}
}

unsigned UsbSerial::getStatus()
{
	unsigned res = status;
	status = 0;

	// TODO: Is it possible for rx fifo to overflow? Or does USB throttle?
	// if(ustat & UART_STATUS_RXFIFO_OVF) {
	// 	bitSet(status, eSERS_Overflow);
	// }

	return res;
}

} // namespace USB::CDC

#endif
