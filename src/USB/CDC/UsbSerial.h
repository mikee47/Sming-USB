#pragma once

#include <HardwareSerial.h>
#include <SimpleTimer.h>
#include <Data/BitSet.h>
#include <memory>

namespace USB::CDC
{
enum class Event {
	rx_data,
	tx_done,
	line_break,
};

class UsbSerial : public ReadWriteStream
{
public:
	using DataReceived = StreamDataReceivedDelegate;
	using TransmitComplete = Delegate<void(UsbSerial& device)>;

	UsbSerial();
	~UsbSerial();

	/**
	 * @brief Sets receiving buffer size
	 * @param size requested size
	 * @retval size_t actual size
	 */
	virtual size_t setRxBufferSize(size_t size) = 0;

	/**
	 * @brief Sets transmit buffer size
	 * @param size requested size
	 * @retval size_t actual size
	 */
	virtual size_t setTxBufferSize(size_t size) = 0;

	/**
	 * @brief Governs write behaviour when UART transmit buffers are full
	 * @param wait
	 * If false, writes will return short count; applications can use the txComplete callback to send more data.
	 * If true, writes will wait for more buffer space so that all requested data is written
	 */
	void setTxWait(bool wait)
	{
		bitWrite(options, UART_OPT_TXWAIT, wait);
	}

	uint16_t readMemoryBlock(char* buf, int max_len) override
	{
		return readBytes(buf, max_len);
	}

	bool seek(int len) override
	{
		return false;
	}

	bool isFinished() override
	{
		return false;
	}

	/** @brief  Clear the serial port transmit/receive buffers
	 * 	@param mode Whether to flush TX, RX or both (the default)
 	 *  @note All un-read buffered data is removed and any error condition cleared
	 */
	virtual void clear(SerialMode mode = SERIAL_FULL) = 0;

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

	bool onDataReceived(DataReceived callback)
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
	 * @brief Get status error flags and clear them
	 * @retval unsigned Status flags, combination of SerialStatus bits
	 * @see SerialStatus
	 */
	unsigned getStatus();

	// Called internally
	void handleEvent(Event event);

protected:
	uart_options_t options{_BV(UART_OPT_TXWAIT)};
	SimpleTimer flushTimer;

private:
	void processEvents();

	DataReceived receiveCallback;
	TransmitComplete transmitCompleteCallback;
	std::unique_ptr<CommandExecutor> commandExecutor;
	uint16_t status{0};
	BitSet<uint8_t, Event> eventMask;
};

} // namespace USB::CDC
