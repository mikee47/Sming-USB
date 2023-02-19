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

namespace USB::CDC
{
class Serial;

/** @brief  Delegate callback type for serial data reception
 *  @param  source Reference to serial stream
 *  @param  arrivedChar Char received
 *  @param  availableCharsCount Quantity of chars available stream in receive buffer
 *  @note Delegate constructor usage: (&YourClass::method, this)
 *
 * 	This delegate is invoked when the serial receive buffer is full, or it times out. The
 * 	arrivedChar indicates the last character received, which might be a '\n' line ending
 * 	character, for example.
 *
 * 	If no receive buffer has been allocated, or it's not big enough to contain the full message,
 * 	then this value will be incorrect as data is stored in the hardware FIFO until read out.
 */
using StreamDataReceivedDelegate = Delegate<void(Stream& source, char arrivedChar, uint16_t availableCharsCount)>;

/** @brief Delegate callback type for serial data transmit completion
 *  @note Invoked when the last byte has left the hardware FIFO
 */
using TransmitCompleteDelegate = Delegate<void(Serial& serial)>;

class Serial : public ReadWriteStream
{
public:
	Serial(int uartPort) : uartNr(uartPort)
	{
	}

	~Serial();

	void setPort(int uartPort)
	{
		end();
		uartNr = uartPort;
	}

	int getPort()
	{
		return uartNr;
	}

	bool begin();

	void end();

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
		smg_uart_set_options(uart, options);
	}

	/** @brief  Get quantity characters available from serial input
     *  @retval int Quantity of characters in receive buffer
     */
	int available() override
	{
		return (int)smg_uart_rx_available(uart);
	}

	/** @brief  Read a character from serial port
     *  @retval int Character read from serial port or -1 if buffer empty
     *  @note   The character is removed from the serial port input buffer
    */
	int read() override
	{
		return smg_uart_read_char(uart);
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
		return smg_uart_read(uart, buf, max_len);
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
		return smg_uart_peek_char(uart);
	}

	/** @brief  Clear the serial port transmit/receive buffers
	 * 	@param mode Whether to flush TX, RX or both (the default)
 	 *  @note All un-read buffered data is removed and any error condition cleared
	 */
	void clear(SerialMode mode = SERIAL_FULL)
	{
		smg_uart_flush(uart, smg_uart_mode_t(mode));
	}

	/** @brief Flush all pending data to the serial port
	 *  @note Not to be confused with uart_flush() which is different. See clear() method.
	 */
	void flush() override // Stream
	{
		smg_uart_wait_tx_empty(uart);
	}

	using Stream::write;

	/** @brief  write multiple characters to serial port
	 *  @param buffer data to write
	 *  @param size number of characters to write
	 *  @retval size_t Quantity of characters written, may be less than size
	 */
	size_t write(const uint8_t* buffer, size_t size) override
	{
		return smg_uart_write(uart, buffer, size);
	}

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
	bool onDataReceived(StreamDataReceivedDelegate dataReceivedDelegate)
	{
		this->HWSDelegate = dataReceivedDelegate;
		return updateUartCallback();
	}

	/** @brief  Set handler for received data
	 *  @param  transmitCompleteDelegate Function to handle received data
	 *  @retval bool Returns true if the callback was set correctly
	 */
	bool onTransmitComplete(TransmitCompleteDelegate transmitCompleteDelegate)
	{
		this->transmitComplete = transmitCompleteDelegate;
		return updateUartCallback();
	}

	/**
	 * @brief Operator that returns true if the uart structure is set
	 */
	operator bool() const
	{
		return uart != nullptr;
	}

	/**
	 * @brief Returns the location of the searched character
	 * @param c - character to search for
	 * @retval int -1 if not found 0 or positive number otherwise
	 */
	int indexOf(char c) override
	{
		return smg_uart_rx_find(uart, c);
	}

	/**
	 * @brief Get status error flags and clear them
	 * @retval unsigned Status flags, combination of SerialStatus bits
	 * @see SerialStatus
	 */
	unsigned getStatus();

private:
	int uartNr = UART_NO;
	TransmitCompleteDelegate transmitComplete = nullptr; ///< Callback for transmit completion
	StreamDataReceivedDelegate HWSDelegate = nullptr;	///< Callback for received data
	CommandExecutor* commandExecutor = nullptr;			 ///< Callback for command execution (received data)
	uart_options_t options = _BV(UART_OPT_TXWAIT);
	size_t txSize = DEFAULT_TX_BUFFER_SIZE;
	size_t rxSize = DEFAULT_RX_BUFFER_SIZE;
	volatile uint16_t statusMask = 0;	 ///< Which serial events require a callback
	volatile uint16_t callbackStatus = 0; ///< Persistent uart status flags for callback
	volatile bool callbackQueued = false;

	static void IRAM_ATTR staticCallbackHandler(smg_uart_t* uart, uint32_t status);
	static void staticOnStatusChange(void* param);
	void invokeCallbacks();

	/**
	 * @brief Called whenever one of the user callbacks change
	 * @retval true if uart callback is active
	 */
	bool updateUartCallback();
};

} // namespace USB::CDC
