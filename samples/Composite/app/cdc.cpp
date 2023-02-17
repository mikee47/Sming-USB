#include "usbdev.h"
#include <SimpleTimer.h>

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
	// FUNC()
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
	FUNC()
}

// Invoked when a TX is complete and therefore space becomes available in TX buffer
void tud_cdc_tx_complete_cb(uint8_t itf)
{
	// FUNC()

	if(itf == 0) {
		writeCDC(itf);
	}
}

// Invoked when line state DTR & RTS are changed via SET_CONTROL_LINE_STATE
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
	FUNC()

	debug_i("DTR %u, RTS %u", dtr, rts);
}

// Invoked when line coding is change via SET_LINE_CODING
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding)
{
	FUNC()
}

// Invoked when received send break
void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms)
{
	FUNC()
}
