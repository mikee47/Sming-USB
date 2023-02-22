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

static const uint8_t conv_table[128][2] = {HID_ASCII_TO_KEYCODE};
static const char* testText = "\x1b echo This should be harmless enough... Rrrrepeatinggggg.\n";
static char lastChar;
static unsigned charIndex;

void sendChar()
{
	char c = testText[charIndex];
	if(c == lastChar) {
		lastChar = '\0';
		hid_keyboard_report_t report{};
		USB::hid0.sendReport(REPORT_ID_KEYBOARD, &report, sizeof(report), sendChar);
		return;
	}
	++charIndex;
	lastChar = c;
	auto& entry = conv_table[unsigned(c)];
	hid_keyboard_report_t report{
		uint8_t(entry[0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0),
		.reserved = 0,
		.keycode = {entry[1]},
	};

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
	Serial << +_F("TinyUSB Host CDC MSC HID Example") << endl;

	bool res = USB::begin();
	debug_i("USB::begin(): %u", res);

#if CFG_TUD_CDC
	USB::cdc0.systemDebugOutput(true);
	USB::cdc0.onDataReceived([](Stream& stream, char arrivedChar, unsigned short availableCharsCount) {
		char buf[availableCharsCount];
		auto n = stream.readBytes(buf, availableCharsCount);
		Serial.write(buf, n);
	});
	Serial.onDataReceived([](Stream& stream, char arrivedChar, unsigned short availableCharsCount) {
		Serial.read();
		System.queueCallback([](uint32_t param) { Serial.write(char(param)); }, arrivedChar);
		return;
		for(;;) {
			char buf[512];
			auto n = stream.readBytes(buf, sizeof(buf));
			if(n == 0) {
				break;
			}
			Serial.write(buf, n);
			// USB::cdc0.write(buf, n);
		}
	});
#endif

#if CFG_TUD_MSC
	USB::msc0.add(Storage::spiFlash, true);
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
		// sendText();
	}));
	timer.start();
}
