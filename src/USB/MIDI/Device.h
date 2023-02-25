#pragma once

#include "../Interface.h"

namespace USB::MIDI
{
union Packet {
	uint8_t data[4];
	struct {
		uint8_t code : 4;
		uint8_t cable_number : 4;
		uint8_t m0;
		uint8_t m1;
		uint8_t m2;
	};
};

enum class Event {
	rx,
};

class Device : public Interface
{
public:
	using Callback = Delegate<void()>;

	Device(uint8_t instance, const char* name);

	// Check if midi interface is mounted
	bool isMounted() const
	{
		return tud_midi_n_mounted(inst);
	}

	// Get the number of bytes available for reading
	uint32_t available(uint8_t cable_num) const
	{
		return tud_midi_n_available(inst, cable_num);
	}

	// Read byte stream (legacy)
	uint32_t streamRead(uint8_t cable_num, void* buffer, uint32_t bufsize)
	{
		return tud_midi_n_stream_read(inst, cable_num, buffer, bufsize);
	}

	// Write byte Stream (legacy)
	uint32_t streamWrite(uint8_t cable_num, uint8_t const* buffer, uint32_t bufsize)
	{
		return tud_midi_n_stream_write(inst, cable_num, buffer, bufsize);
	}

	// Read event packet
	bool readPacket(Packet& packet)
	{
		return tud_midi_n_packet_read(inst, packet.data);
	}

	// Write event packet
	bool writePacket(const Packet& packet)
	{
		return tud_midi_n_packet_write(inst, packet.data);
	}

	void onDataReceived(Callback callback)
	{
		receiveCallback = callback;
	}

	void handle_event(Event event);

private:
	Callback receiveCallback;
	uint8_t inst;
};

} // namespace USB::MIDI
