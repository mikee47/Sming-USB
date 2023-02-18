#include "usbdev.h"
#include <Storage/SpiFlash.h>

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
	(void)lun;

	const char vid[] = "TinyUSB";
	const char pid[] = "Mass Storage";
	const char rev[] = "1.0";

	memcpy(vendor_id, vid, strlen(vid));
	memcpy(product_id, pid, strlen(pid));
	memcpy(product_rev, rev, strlen(rev));

	debug_i("%s(%u, \"%s\", \"%s\", \"%s\")", __FUNCTION__, lun, vid, pid, rev);
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
	debug_i("%s(%u, %u)", __FUNCTION__, lun, bufsize);
	return -1;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
	*block_size = Storage::spiFlash->getSectorSize();
	*block_count = Storage::spiFlash->getSectorCount();

	debug_i("%s(%u, %u, %u)", __FUNCTION__, lun, *block_count, *block_size);
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
	debug_i("%s(%u)", __FUNCTION__, lun);
	(void)lun;

	return (lun == 0);
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
	if(lun != 0) {
		return -1;
	}

	auto blockSize = Storage::spiFlash->getSectorSize();
	offset += blockSize * lba;
	return Storage::spiFlash->read(offset, buffer, bufsize) ? bufsize : -1;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
	debug_i("%s(%u, %u)", __FUNCTION__, lun);
	(void)lun;

	return false;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
	(void)lun;

	return -1;
}
