#include <USB.h>

#if defined(ENABLE_USB_CLASSES) && CFG_TUD_MSC

#include <debug_progmem.h>

namespace USB::MSC
{
LogicalUnit Device::logicalUnits[MAX_LUN];

bool Device::setLogicalUnit(uint8_t lun, LogicalUnit unit)
{
	if(lun >= MAX_LUN) {
		debug_e("[MSC] Invalid LUN %u", lun);
		return false;
	}

	logicalUnits[lun] = unit;
	return true;
}

void Device::inquiry(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
	auto unit = Device::getLogicalUnit(lun);
	if(!unit) {
		return;
	}

	const char vid[] = "TinyUSB";
	const char rev[] = "1.0";

	memcpy(vendor_id, vid, strlen(vid));
	String devname = unit.device->getName();
	memcpy(product_id, devname.c_str(), std::min(devname.length(), 16U));
	memcpy(product_rev, rev, strlen(rev));

	debug_i("%s(%u, \"%s\", \"%s\", \"%s\")", __FUNCTION__, lun, vid, devname.c_str(), rev);
}

} // namespace USB::MSC

using namespace USB::MSC;

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
	return Device::inquiry(lun, vendor_id, product_id, product_rev);
}

// Invoked when received GET_MAX_LUN request, required for multiple LUNs implementation
uint8_t tud_msc_get_maxlun_cb(void)
{
	return Device::MAX_LUN;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
// bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
// {
// }

// Invoked when received REQUEST_SENSE
// int32_t tud_msc_request_sense_cb(uint8_t lun, void* buffer, uint16_t bufsize)
// {
// }

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
	return Device::getCapacity(lun, block_count, block_size);
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
	return Device::isReady(lun);
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
	return Device::read(lun, lba, offset, buffer, bufsize);
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
	return !Device::isReadOnly(lun);
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
	return Device::write(lun, lba, offset, buffer, bufsize);
}

#endif
