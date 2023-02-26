#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_CDC

#include "Platform/System.h"
#include <SimpleTimer.h>

#if ENABLE_CMD_EXECUTOR
#include <Services/CommandProcessing/CommandExecutor.h>
#endif

namespace USB::CDC
{
class InternalDevice : public Device
{
public:
	using Device::handleEvent;
};

InternalDevice* getDevice(uint8_t inst)
{
	extern InternalDevice* devices[];
	return (inst < CFG_TUD_CDC) ? devices[inst] : nullptr;
}

Device::Device(uint8_t instance, const char* name) : Interface(instance, name)
{
	flushTimer.initializeMs<50>(
		[](void* param) {
			auto self = static_cast<Device*>(param);
			self->flush();
		},
		this);
}

Device::~Device()
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

	flushTimer.startOnce();

	return written;
}

void Device::handleEvent(Event event)
{
	if(event == Event::line_break) {
		bitSet(status, eSERS_BreakDetected);
		return;
	}

	if(!eventMask) {
		System.queueCallback(
			[](void* param) {
				auto self = static_cast<Device*>(param);
				self->processEvents();
			},
			this);
	}

	eventMask += event;
}

void Device::processEvents()
{
	auto evt = eventMask;
	eventMask = 0;
	if(evt[Event::rx_data]) {
		if(receiveCallback) {
			uint8_t ch{0};
			tud_cdc_n_peek(inst, &ch);
			receiveCallback(*this, ch, tud_cdc_n_available(inst));
		}
#if ENABLE_CMD_EXECUTOR
		if(commandExecutor) {
			uint8_t ch;
			while(tud_cdc_n_read(inst, &ch, 1)) {
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

void Device::systemDebugOutput(bool enabled)
{
	if(enabled) {
		m_setPuts([this](const char* data, size_t length) -> size_t { return write(data, length); });
	} else {
		m_setPuts(nullptr);
	}
}

unsigned Device::getStatus()
{
	unsigned res = status;
	status = 0;

	// TODO: Is it possible for rx fifo to overflow? Or does USB throttle?
	// if(ustat & UART_STATUS_RXFIFO_OVF) {
	// 	bitSet(status, eSERS_Overflow);
	// }

	return res;
}

void Device::commandProcessing(bool reqEnable)
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
