/****
 * MIDI/Device.cpp
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

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_MIDI

namespace USB::MIDI
{
class InternalDevice : public Device
{
public:
	using Device::handleEvent;
};

InternalDevice* getDevice(uint8_t inst)
{
	extern InternalDevice* devices[];
	return (inst < CFG_TUD_MIDI) ? devices[inst] : nullptr;
}

/*
 * Ordinarily this constructor code can live in the header.
 * However, the linker then doesn't bother with this file so tud_midi_rx_cb remains undefined.
 * The problem is also difficult to detect during compile time.
 * One reason to avoid weak symbols if possible.
 */
Device::Device(uint8_t instance, const char* name) : DeviceInterface(instance, name)
{
}

void Device::handleEvent(Event event)
{
	switch(event) {
	case Event::rx:
		if(receiveCallback) {
			receiveCallback();
		}
	}
}

} // namespace USB::MIDI

using namespace USB::MIDI;

void tud_midi_rx_cb(uint8_t itf)
{
	auto dev = getDevice(itf);
	if(dev) {
		dev->handleEvent(Event::rx);
	}
}

#endif
