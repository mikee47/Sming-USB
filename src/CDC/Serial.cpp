#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_MSC

// #include <cstdarg>
#include "Platform/System.h"
// #include "m_printf.h"
#include <SimpleTimer.h>

#if ENABLE_CMD_EXECUTOR
#include <Services/CommandProcessing/CommandExecutor.h>
#endif


namespace
{
/*
 * FIFO doesn't get written until flushed, so use a timer to handle that.
 */
String cdcBuffer;
SimpleTimer* cdcTimer;

} // namespace

static void writeCDC(uint8_t itf)
{
	auto toWrite = cdcBuffer.length();
	if(toWrite == 0) {
		return;
	}
	auto written = tud_cdc_n_write(itf, cdcBuffer.c_str(), toWrite);
	// debug_i("tud_cdc_n_write(%u, %u): %u", itf, toWrite, written);
	cdcBuffer.remove(0, written);
}

// Invoked when received new data
void tud_cdc_rx_cb(uint8_t itf)
{
	// debug_i("cdc itf = %u", itf);

	char buf[64];
	auto count = tud_cdc_n_read(itf, buf, sizeof(buf));
	// debug_hex(INFO, "CDC", buf, count);

	if(itf != 0) {
		return;
	}

	cdcBuffer.concat(buf, count);

	writeCDC(0);

	if(cdcTimer == nullptr) {
		cdcTimer = new SimpleTimer;
		cdcTimer->initializeMs<50>(InterruptCallback([] { tud_cdc_n_write_flush(0); }));
	}
	cdcTimer->startOnce();
}

// Invoked when received `wanted_char`
void tud_cdc_rx_wanted_cb(uint8_t itf, char wanted_char)
{
	debug_i("%s(%u, %u)", __FUNCTION__, itf, wanted_char);
}

// Invoked when a TX is complete and therefore space becomes available in TX buffer
void tud_cdc_tx_complete_cb(uint8_t itf)
{
	if(itf == 0) {
		writeCDC(itf);
	}
}

// Invoked when line state DTR & RTS are changed via SET_CONTROL_LINE_STATE
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
	debug_i("%s(%u, DTR %u, RTS %u)", __FUNCTION__, itf, dtr, rts);
}

// Invoked when line coding is change via SET_LINE_CODING
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding)
{
	debug_i("%s(%u, %u, %u-%u-%u)", __FUNCTION__, itf, p_line_coding->bit_rate, p_line_coding->data_bits,
			p_line_coding->parity, p_line_coding->stop_bits);
}

// Invoked when received send break
void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms)
{
	debug_i("%s(%u, %u)", __FUNCTION__, itf, duration_ms);
}

namespace USB::CDC
{
bool Serial::begin(uint32_t baud, SerialFormat format, SerialMode mode, uint8_t txPin, uint8_t rxPin)
{
	end();

	if(uartNr < 0) {
		return false;
	}

	smg_uart_config_t cfg = {
		.uart_nr = (uint8_t)uartNr,
		.tx_pin = txPin,
		.rx_pin = rxPin,
		.mode = smg_uart_mode_t(mode),
		.options = options,
		.baudrate = baud,
		.format = smg_uart_format_t(format),
		.rx_size = rxSize,
		.tx_size = txSize,
	};
	uart = smg_uart_init_ex(cfg);
	updateUartCallback();

	return uart != nullptr;
}

void Serial::end()
{
	if(uart == nullptr) {
		return;
	}

	smg_uart_uninit(uart);
	uart = nullptr;
}

size_t Serial::setRxBufferSize(size_t size)
{
	if(uart) {
		rxSize = smg_uart_resize_rx_buffer(uart, size);
	} else {
		rxSize = size;
	}
	return rxSize;
}

size_t Serial::setTxBufferSize(size_t size)
{
	if(uart) {
		txSize = smg_uart_resize_tx_buffer(uart, size);
	} else {
		txSize = size;
	}
	return txSize;
}

void Serial::systemDebugOutput(bool enabled)
{
	if(!uart) {
		return;
	}

	if(enabled) {
		if(smg_uart_tx_enabled(uart)) {
			smg_uart_set_debug(uartNr);
			m_setPuts(std::bind(&smg_uart_write, uart, _1, _2));
		} else {
			smg_uart_set_debug(UART_NO);
		}
	} else if(smg_uart_get_debug() == uartNr) {
		// Disable system debug messages on this interface
		smg_uart_set_debug(UART_NO);
		// and don't print debugf() data at all
		m_setPuts(nullptr);
	}
}

void Serial::invokeCallbacks()
{
	(void)smg_uart_disable_interrupts();
	auto status = callbackStatus;
	callbackStatus = 0;
	callbackQueued = false;
	smg_uart_restore_interrupts();

	// Transmit complete ?
	if((status & UART_STATUS_TXFIFO_EMPTY) != 0 && transmitComplete) {
		transmitComplete(*this);
	}

	// RX FIFO Full or RX FIFO Timeout or RX Overflow ?
	if(status & (UART_STATUS_RXFIFO_FULL | UART_STATUS_RXFIFO_TOUT | UART_STATUS_RXFIFO_OVF)) {
		auto receivedChar = smg_uart_peek_last_char(uart);
		if(HWSDelegate) {
			HWSDelegate(*this, receivedChar, smg_uart_rx_available(uart));
		}
#if ENABLE_CMD_EXECUTOR
		if(commandExecutor) {
			int c;
			while((c = smg_uart_read_char(uart)) >= 0) {
				commandExecutor->executorReceive(c);
			}
		}
#endif
	}
}

unsigned Serial::getStatus()
{
	unsigned status = 0;
	unsigned ustat = smg_uart_get_status(uart);
	if(ustat & UART_STATUS_BRK_DET) {
		bitSet(status, eSERS_BreakDetected);
	}

	if(ustat & UART_STATUS_RXFIFO_OVF) {
		bitSet(status, eSERS_Overflow);
	}

	if(ustat & UART_STATUS_FRM_ERR) {
		bitSet(status, eSERS_FramingError);
	}

	if(ustat & UART_STATUS_PARITY_ERR) {
		bitSet(status, eSERS_ParityError);
	}

	return status;
}

/*
 * Called via task queue
 */
void Serial::staticOnStatusChange(void* param)
{
	auto serial = static_cast<Serial*>(param);
	if(serial != nullptr) {
		serial->invokeCallbacks();
	}
}

/*
 * Called from uart interrupt handler
 */
void Serial::staticCallbackHandler(smg_uart_t* uart, uint32_t status)
{
	auto serial = static_cast<Serial*>(smg_uart_get_callback_param(uart));
	if(serial == nullptr) {
		return;
	}

	serial->callbackStatus |= status;

	// If required, queue a callback
	if((status & serial->statusMask) != 0 && !serial->callbackQueued) {
		System.queueCallback(staticOnStatusChange, serial);
		serial->callbackQueued = true;
	}
}

bool Serial::updateUartCallback()
{
	uint16_t mask = 0;
#if ENABLE_CMD_EXECUTOR
	if(HWSDelegate || commandExecutor) {
#else
	if(HWSDelegate) {
#endif
		mask |= UART_STATUS_RXFIFO_FULL | UART_STATUS_RXFIFO_TOUT | UART_STATUS_RXFIFO_OVF;
	}

	if(transmitComplete) {
		mask |= UART_STATUS_TXFIFO_EMPTY;
	}

	statusMask = mask;

	setUartCallback(mask == 0 ? nullptr : staticCallbackHandler, this);

	return mask != 0;
}

void Serial::commandProcessing(bool reqEnable)
{
#if ENABLE_CMD_EXECUTOR
	if(reqEnable) {
		if(!commandExecutor) {
			commandExecutor = new CommandExecutor(this);
		}
	} else {
		delete commandExecutor;
		commandExecutor = nullptr;
	}
	updateUartCallback();
#endif
}

} // namespace USB::CDC

#endif
