#include <SmingCore.h>
#include <USB.h>
#include <Storage/SpiFlash.h>

static SimpleTimer timer;

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

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true);

	delay(1000);
	Serial << +_F("TinyUSB Host CDC MSC HID Example") << endl;

	bool res = USB::begin();
	debug_i("USB::begin(): %u", res);

	// USB::cdc0.systemDebugOutput(true);
	USB::cdc0.onDataReceived([](Stream& stream, char arrivedChar, unsigned short availableCharsCount) {
		debug_e("arrivedchar: %c, avail = %d", arrivedChar, availableCharsCount);
		char buf[availableCharsCount];
		auto n = stream.readBytes(buf, availableCharsCount);
		debug_i("read %u of %u", n, availableCharsCount);
		Serial.write(buf, n);
		Serial << endl;
	});
	Serial.onDataReceived([](Stream& stream, char arrivedChar, unsigned short availableCharsCount) {
		for(;;) {
			char buf[512];
			auto n = stream.readBytes(buf, sizeof(buf));
			if(n == 0) {
				break;
			}
			// USB::cdc0.write(buf, n);
		}
	});

	USB::msc0.add(Storage::spiFlash, true);

	timer.initializeMs<3000>(InterruptCallback([]() { debug_i("Alive"); }));
	timer.start();
}
