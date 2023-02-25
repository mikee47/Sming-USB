#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_MIDI

namespace USB::MIDI
{
Device* getDevice(uint8_t inst)
{
	extern Device* devices[];
	return (inst < CFG_TUD_MIDI) ? devices[inst] : nullptr;
}

/*
 * Ordinarily this constructor code can live in the header.
 * However, the linker then doesn't bother with this file so tud_midi_rx_cb remains undefined.
 * The problem is also difficult to detect during compile time.
 * One reason to avoid weak symbols if possible.
 */
Device::Device(uint8_t instance, const char* name) : Interface(instance, name)
{
}

void Device::handle_event(Event event)
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
		dev->handle_event(Event::rx);
	}
}

#endif
