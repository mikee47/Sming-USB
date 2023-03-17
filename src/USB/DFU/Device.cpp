#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_DFU

#include <FlashString/Vector.hpp>

namespace
{
USB::DFU::Callbacks* callbacks;
} // namespace

namespace USB::DFU
{
Device::Device(uint8_t inst, const char* name) : DeviceInterface(inst, name)
{
}

void Device::begin(Callbacks& cb)
{
	callbacks = &cb;
}

} // namespace USB::DFU

using namespace USB::DFU;
using Alternate = Callbacks::Alternate;

uint32_t tud_dfu_get_timeout_cb(uint8_t alt, uint8_t state)
{
	return callbacks ? callbacks->getTimeout(Alternate(alt), dfu_state_t(state)) : 0;
}

void tud_dfu_download_cb(uint8_t alt, uint16_t block_num, uint8_t const* data, uint16_t length)
{
	if(callbacks) {
		callbacks->download(Alternate(alt), uint32_t(block_num) * CFG_TUD_DFU_XFER_BUFSIZE, data, length);
	}
}

void tud_dfu_manifest_cb(uint8_t alt)
{
	if(callbacks) {
		callbacks->manifest(Alternate(alt));
	}
}

uint16_t tud_dfu_upload_cb(uint8_t alt, uint16_t block_num, uint8_t* data, uint16_t length)
{
	return callbacks ? callbacks->upload(Alternate(alt), uint32_t(block_num) * CFG_TUD_DFU_XFER_BUFSIZE, data, length)
					 : 0;
}

void tud_dfu_abort_cb(uint8_t alt)
{
	if(callbacks) {
		callbacks->abort(Alternate(alt));
	}
}

void tud_dfu_detach_cb(void)
{
	if(callbacks) {
		callbacks->detach();
	}
}

#endif
