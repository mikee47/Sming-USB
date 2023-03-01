#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_DFU

#include <FlashString/Vector.hpp>

namespace
{
DEFINE_FSTR(image1, "Hello world from TinyUSB DFU! - Partition 0")
DEFINE_FSTR(image2, "Hello world from TinyUSB DFU! - Partition 1")
DEFINE_FSTR(image3, "Hello world from TinyUSB DFU! - Partition 3")
DEFINE_FSTR_VECTOR(upload_images, FlashString, &image1, &image2, &image3)
} // namespace

namespace USB::DFU
{
Device::Device(uint8_t inst, const char* name) : DeviceInterface(inst, name)
{
}

} // namespace USB::DFU

//--------------------------------------------------------------------+
// DFU callbacks
// Note: alt is used as the partition number, in order to support multiple partitions like FLASH, EEPROM, etc.
//--------------------------------------------------------------------+

// Invoked right before tud_dfu_download_cb() (state=DFU_DNBUSY) or tud_dfu_manifest_cb() (state=DFU_MANIFEST)
// Application return timeout in milliseconds (bwPollTimeout) for the next download/manifest operation.
// During this period, USB host won't try to communicate with us.
uint32_t tud_dfu_get_timeout_cb(uint8_t alt, uint8_t state)
{
	if(state == DFU_DNBUSY) {
		// For this example
		// - Atl0 Flash is fast : 1   ms
		// - Alt1 EEPROM is slow: 100 ms
		return (alt == 0) ? 1 : 100;
	} else if(state == DFU_MANIFEST) {
		// since we don't buffer entire image and do any flashing in manifest stage
		return 0;
	}

	return 0;
}

// Invoked when received DFU_DNLOAD (wLength>0) following by DFU_GETSTATUS (state=DFU_DNBUSY) requests
// This callback could be returned before flashing op is complete (async).
// Once finished flashing, application must call tud_dfu_finish_flashing()
void tud_dfu_download_cb(uint8_t alt, uint16_t block_num, uint8_t const* data, uint16_t length)
{
	(void)alt;
	(void)block_num;

	debug_i("[DFU] Received Alt %u BlockNum %u of length %u", alt, block_num, length);
	debug_hex(INFO, "DFU", data, length);

	// flashing op for download complete without error
	tud_dfu_finish_flashing(DFU_STATUS_OK);
}

// Invoked when download process is complete, received DFU_DNLOAD (wLength=0) following by DFU_GETSTATUS (state=Manifest)
// Application can do checksum, or actual flashing if buffered entire image previously.
// Once finished flashing, application must call tud_dfu_finish_flashing()
void tud_dfu_manifest_cb(uint8_t alt)
{
	(void)alt;
	debug_i("[DFU] Download completed, enter manifestation");

	// flashing op for manifest is complete without error
	// Application can perform checksum, should it fail, use appropriate status such as errVERIFY.
	tud_dfu_finish_flashing(DFU_STATUS_OK);
}

// Invoked when received DFU_UPLOAD request
// Application must populate data with up to length bytes and
// Return the number of written bytes
uint16_t tud_dfu_upload_cb(uint8_t alt, uint16_t block_num, uint8_t* data, uint16_t length)
{
	debug_i("[DFU] Send Alt %u BlockNum %u of length %u", alt, block_num, length);
	return block_num ? 0 : upload_images[alt].read(0, reinterpret_cast<char*>(data), length);
}

// Invoked when the Host has terminated a download or upload transfer
void tud_dfu_abort_cb(uint8_t alt)
{
	(void)alt;
	debug_w("[DFU] Host aborted transfer");
}

// Invoked when a DFU_DETACH request is received
void tud_dfu_detach_cb(void)
{
	debug_i("[DFU] Host detach, we should probably reboot");
}

#endif
