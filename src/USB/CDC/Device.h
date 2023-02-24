/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * USB/CDC/Serial.h
 *
 ****/

#pragma once

#include <HardwareSerial.h>
#include <SimpleTimer.h>
#include <Data/BitSet.h>
#include <memory>

namespace USB::CDC
{
class Device : public ReadWriteStream
{
public:
	using TransmitComplete = Delegate<void(Device& device)>;

	Device(uint8_t instance, const char* name);
	~Device();

	/**
	 * @brief Sets receiving buffer size
	 * @param size requested size
	 * @retval size_t actual size
	 */
	size_t setRxBufferSize(size_t size);

	/**
	 * @brief Sets transmit buffer size
	 * @param size requested size
	 * @retval size_t actual size
	 */
	size_t setTxBufferSize(size_t size);

	/**
	 * @brief Governs write behaviour when UART transmit buffers are full
	 * @param wait
	 * If false, writes will return short count; applications can use the txComplete callback to send more data.
	 * If true, writes will wait for more buffer space so that all requested data is written
	 */
	void setTxWait(bool wait)
	{
		bitWrite(options, UART_OPT_TXWAIT, wait);
		// smg_uart_set_options(uart, options);
	}

	/** @brief  Get quantity characters available from serial input
     *  @retval int Quantity of characters in receive buffer
     */
	int available() override
	{
		return tud_cdc_n_available(inst);
	}

	/** @brief  Read a character from serial port
     *  @retval int Character read from serial port or -1 if buffer empty
     *  @note   The character is removed from the serial port input buffer
    */
	int read() override
	{
		return tud_cdc_n_read_char(inst);
	}

	/** @brief  Read a block of characters from serial port
	 *  @param  buf Pointer to buffer to hold received data
	 *  @param  max_len Maximum quantity of characters to read
	 *  @retval uint16_t Quantity of characters read
	 *  @note Although this shares the same name as the method in IDataSourceStream,
	 *  behaviour is different because in effect the 'seek' position is changed by this call.
	 */
	uint16_t readMemoryBlock(char* buf, int max_len) override
	{
		return tud_cdc_n_read(inst, buf, max_len);
	}

	bool seek(int len) override
	{
		return false;
	}

	bool isFinished() override
	{
		return false;
	}

	/** @brief  Read a character from serial port without removing from input buffer
     *  @retval int Character read from serial port or -1 if buffer empty
     *  @note   The character remains in serial port input buffer
     */
	int peek() override
	{
		uint8_t c;
		return tud_cdc_n_peek(inst, &c) ? c : -1;
	}

	/** @brief  Clear the serial port transmit/receive buffers
	 * 	@param mode Whether to flush TX, RX or both (the default)
 	 *  @note All un-read buffered data is removed and any error condition cleared
	 */
	void clear(SerialMode mode = SERIAL_FULL)
	{
		if(mode != SerialMode::TxOnly) {
			tud_cdc_n_read_flush(inst);
		}
		if(mode != SerialMode::RxOnly) {
			tud_cdc_n_write_clear(inst);
		}
	}

	/** @brief Flush all pending data to the serial port
	 *  @note Not to be confused with uart_flush() which is different. See clear() method.
	 */
	void flush() override // Stream
	{
		tud_cdc_n_write_flush(inst);
	}

	using Stream::write;

	/** @brief  write multiple characters to serial port
	 *  @param buffer data to write
	 *  @param size number of characters to write
	 *  @retval size_t Quantity of characters written, may be less than size
	 */
	size_t write(const uint8_t* buffer, size_t size) override;

	/** @brief  Configure serial port for system debug output and redirect output from debugf
	 *  @param  enabled True to enable this port for system debug output
	 *  @note   If enabled, port will issue system debug messages
	 */
	void systemDebugOutput(bool enabled);

	/** @brief  Configure serial port for command processing
	 *  @param  reqEnable True to enable command processing
	 *  @note   Command processing provides a CLI to the system
	 *  @see    commandHandler
	 */
	void commandProcessing(bool reqEnable);

	/** @brief  Set handler for received data
	 *  @param  dataReceivedDelegate Function to handle received data
	 *  @retval bool Returns true if the callback was set correctly
	 */
	bool onDataReceived(StreamDataReceivedDelegate callback)
	{
		receiveCallback = callback;
		return true;
	}

	bool onTransmitComplete(TransmitComplete callback)
	{
		transmitCompleteCallback = callback;
		return true;
	}

	/**
	 * @brief Returns the location of the searched character
	 * @param c - character to search for
	 * @retval int -1 if not found 0 or positive number otherwise
	 */
	int indexOf(char c) override
	{
		return -1;
	}

	/**
	 * @brief Get status error flags and clear them
	 * @retval unsigned Status flags, combination of SerialStatus bits
	 * @see SerialStatus
	 */
	unsigned getStatus();

	enum class Event {
		rx_data,
		tx_done,
	};
	void queueEvent(Event event);

private:
	void processEvents();

	uint8_t inst;
	StreamDataReceivedDelegate receiveCallback;
	TransmitComplete transmitCompleteCallback;
	std::unique_ptr<CommandExecutor> commandExecutor;
	SimpleTimer flushTimer;
	nputs_callback_t oldPuts{};
	uart_options_t options{_BV(UART_OPT_TXWAIT)};
	BitSet<uint8_t, Event> eventMask;
};

} // namespace USB::CDC
