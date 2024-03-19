#include <SmingCore.h>
#include <USB.h>
#include <Storage/SpiFlash.h>
#include <FlashString/Vector.hpp>

void tuh_mount_cb(uint8_t dev_addr)
{
	// application set-up
	debug_i("A device with address %u is mounted", dev_addr);
}

void tuh_umount_cb(uint8_t dev_addr)
{
	// application tear-down
	debug_i("A device with address %u is unmounted", dev_addr);
}

namespace
{
SimpleTimer timer;

#if CFG_TUD_HID

// Use provided TinyUSB table to convert ASCII codes to HID codes.
// Note that this conversion table does not account for all keyboard layout differences.
const uint8_t conv_table[128][2] = {HID_ASCII_TO_KEYCODE};

// String of text to 'type out' on the HID keyboard
const char* testText = "\x1b echo This should be harmless enough... Rrrrepeatinggggg.\n";

// State variables
char lastChar;
unsigned charIndex;

void sendChar()
{
	hid_keyboard_report_t report{};
	char c = testText[charIndex];
	if(c == lastChar) {
		lastChar = '\0';
	} else {
		++charIndex;
		lastChar = c;
		auto& entry = conv_table[unsigned(c)];
		report.modifier = entry[0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
		report.keycode[0] = entry[1];
	}

	USB::hid0.sendReport(REPORT_ID_KEYBOARD, &report, sizeof(report), c ? sendChar : nullptr);
}

void sendText()
{
	charIndex = 0;
	lastChar = '\0';
	sendChar();
}

#endif // CFG_TUD_HID

#if CFG_TUD_MIDI

const char* midiCodes[] = {
	"MISC",			  "CABLE_EVENT",	  "SYSCOM_2BYTE",	  "SYSCOM_3BYTE",
	"SYSEX_START",	"SYSEX_END_1BYTE",  "SYSEX_END_2BYTE",   "SYSEX_END_3BYTE",
	"NOTE_OFF",		  "NOTE_ON",		  "POLY_KEYPRESS",	 "CONTROL_CHANGE",
	"PROGRAM_CHANGE", "CHANNEL_PRESSURE", "PITCH_BEND_CHANGE", "1BYTE_DATA",
};

#endif

#if CFG_TUD_DFU

DEFINE_FSTR(image1, "Hello world from TinyUSB DFU! - Partition 0")
DEFINE_FSTR(image2, "Hello world from TinyUSB DFU! - Partition 1")
DEFINE_FSTR(image3, "Hello world from TinyUSB DFU! - Partition 2")
DEFINE_FSTR_VECTOR(upload_images, FlashString, &image1, &image2, &image3)

class DfuCallbacks : public USB::DFU::Callbacks
{
public:
	uint32_t getTimeout(Alternate alt, dfu_state_t state) override
	{
		if(state == DFU_DNBUSY) {
			// For this example, EEPROM is slow (100ms), FLASH and RAM are fast (1ms)
			return (alt == DFU_ALTERNATE_EEPROM) ? 100 : 1;
		}

		if(state == DFU_MANIFEST) {
			// since we don't buffer entire image and do any flashing in manifest stage
			return 0;
		}

		return 0;
	}

	void download(Alternate alt, uint32_t offset, const void* data, uint16_t length) override
	{
		debug_i("[DFU] Download alt %u, offset %u, length %u", alt, offset, length);

		auto status = DFU_STATUS_OK;

		switch(alt) {
		case DFU_ALTERNATE_FLASH: {
			auto part = Storage::findPartition("flash0");
			if(!part) {
				status = DFU_STATUS_ERR_TARGET;
				break;
			}
			auto blockSize = part.getBlockSize();
			if(offset % blockSize == 0) {
				if(!part.erase_range(offset, blockSize)) {
					status = DFU_STATUS_ERR_ERASE;
					break;
				}
			}
			if(!part.write(offset, data, length)) {
				status = DFU_STATUS_ERR_WRITE;
			}
			break;
		}

		default:
			debug_hex(INFO, "DFU", data, length);
		}

		USB::dfu0.complete(status);
	}

	void manifest(Alternate alt) override
	{
		debug_i("[DFU] Alt %u download complete, enter manifestation", alt);

		// flashing op for manifest is complete without error
		// Application can perform checksum, should it fail, use appropriate status such as errVERIFY.
		USB::dfu0.complete(DFU_STATUS_OK);
	}

	uint16_t upload(Alternate alt, uint32_t offset, void* data, uint16_t length) override
	{
		uint16_t res{0};
		switch(alt) {
		case DFU_ALTERNATE_FLASH: {
			auto part = Storage::findPartition("flash0");
			res = part.read(offset, data, length) ? length : 0;
			break;
		}
		default:
			res = upload_images[alt].read(offset, reinterpret_cast<char*>(data), length);
		}
		debug_i("[DFU] Upload alt %u, offset %u, length %u; returning %u bytes", alt, offset, length, res);
		return res;
	}

	void abort(Alternate alt) override
	{
		debug_w("[DFU] Host aborted transfer, alt %u", alt);
	}

	void detach() override
	{
		debug_i("[DFU] Host detach, we should probably reboot");
	}
};

DfuCallbacks dfuCallbacks;
#endif

} // namespace

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true);

	delay(1000);
	Serial << +_F("Sming Basic Device USB sample application") << endl;

	bool res = USB::begin();
	debug_i("USB::begin(): %u", res);

	/* Return unique serial number for this SoC */
	USB::onGetDescriptorSting([](uint8_t index) -> const USB::Descriptor* {
		// MUST use a persistent buffer!
		static USB::StringDescriptor<8> desc;

		switch(index) {
		case STRING_INDEX_DEVICE0_SERIAL: {
			desc = String(system_get_chip_id(), HEX, 8);
			return &desc;
		}

		default:
			return nullptr;
		}
	});

#if CFG_TUD_CDC
	// USB::cdc0.systemDebugOutput(true);
	USB::cdc0.onDataReceived([](Stream& stream, char arrivedChar, unsigned short availableCharsCount) {
		char buf[availableCharsCount];
		auto n = stream.readBytes(buf, availableCharsCount);
		Serial.write(buf, n);
	});
	Serial.onDataReceived([](Stream& stream, char arrivedChar, unsigned short availableCharsCount) {
		char buf[availableCharsCount];
		auto n = stream.readBytes(buf, availableCharsCount);
		Serial.write(buf, n);
		USB::cdc1.write(buf, n);
	});
#endif

#if CFG_TUD_MSC
	USB::msc0.setLogicalUnit(0, {Storage::spiFlash, true});
#endif

#if CFG_TUD_MIDI
	USB::midi0.onDataReceived([]() {
		USB::MIDI::Packet pkt;
		while(USB::midi0.readPacket(pkt)) {
			debug_i("MIDI: %u %u %s(%u, %u, %u)", pkt.cable_number, pkt.code, midiCodes[pkt.code], pkt.m0, pkt.m1,
					pkt.m2);
		}
	});
#endif

#if CFG_TUD_DFU
	USB::dfu0.begin(dfuCallbacks);
#endif

	timer.initializeMs<3000>(InterruptCallback([]() {
		debug_i("Alive");
		// Un-comment this to demonstrated how to send keystrokes to the connected PC!
#if CFG_TUD_HID
		sendText();
#endif
	}));
	timer.start();
}
