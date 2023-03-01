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
