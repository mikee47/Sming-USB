#include <SmingCore.h>
#include <USB.h>
#include <Storage/SpiFlash.h>

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

#endif

#if CFG_TUD_MIDI

const char* midiCodes[] = {
	"MISC",			  "CABLE_EVENT",	  "SYSCOM_2BYTE",	  "SYSCOM_3BYTE",
	"SYSEX_START",	"SYSEX_END_1BYTE",  "SYSEX_END_2BYTE",   "SYSEX_END_3BYTE",
	"NOTE_OFF",		  "NOTE_ON",		  "POLY_KEYPRESS",	 "CONTROL_CHANGE",
	"PROGRAM_CHANGE", "CHANNEL_PRESSURE", "PITCH_BEND_CHANGE", "1BYTE_DATA",
};

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

	USB::onGetDescriptorSting([](uint8_t index) -> const USB::Descriptor* {
		switch(index) {
		case STRING_INDEX_DEVICE0_SERIAL: {
			static USB::StringDescriptor<8> desc;
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

	timer.initializeMs<3000>(InterruptCallback([]() {
		debug_i("Alive");
		// Un-comment this to demonstrated how to send keystrokes to the connected PC!
		// sendText();
	}));
	timer.start();
}
