#include <USB.h>

#if defined(ENABLE_USB_CLASSES)

#include "Platform/System.h"
#include <SimpleTimer.h>

#if ENABLE_CMD_EXECUTOR
#include <Services/CommandProcessing/CommandExecutor.h>
#endif

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

UsbSerial::~UsbSerial()
{
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
#if ENABLE_CMD_EXECUTOR
		if(commandExecutor) {
			uint8_t ch;
			while(readBytes(&ch, 1)) {
				commandExecutor->executorReceive(ch);
			}
		}
#endif
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

void UsbSerial::commandProcessing(bool reqEnable)
{
#if ENABLE_CMD_EXECUTOR
	if(reqEnable) {
		if(!commandExecutor) {
			commandExecutor.reset(new CommandExecutor(this));
		}
	} else {
		commandExecutor.reset();
	}
#endif
}

} // namespace USB::CDC

#endif
