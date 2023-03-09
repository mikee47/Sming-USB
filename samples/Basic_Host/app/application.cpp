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
USB::CDC::HostDevice cdc0;
USB::MSC::HostDevice msc0;
USB::HID::HostDevice hid0;

} // namespace

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true);

	delay(1000);
	Serial << +_F("Sming Basic Device USB sample application") << endl;

	bool res = USB::begin();
	debug_i("USB::begin(): %u", res);

#if CFG_TUH_HID
	USB::HID::onMount([](auto& inst, auto report) -> USB::HID::HostDevice* {
		auto protocol = tuh_hid_interface_protocol(inst.dev_addr, inst.idx);
		debug_i("HID mounted, inst %u/%u, protocol %u", inst.dev_addr, inst.idx, protocol);
		m_printHex("RPT", report.desc, report.length);

		tuh_hid_report_info_t info[8];
		auto n = report.parse(info, ARRAY_SIZE(info));
		Serial << "HID has " << n << " reports" << endl;
		for(unsigned i = 0; i < n; ++i) {
			auto& r = info[i];
			Serial << "  ID " << r.report_id << ", Usage " << r.usage << ", Page " << r.usage_page << endl;
		}
		if(protocol != HID_ITF_PROTOCOL_KEYBOARD) {
			return nullptr;
		}
		hid0.begin(inst);
		hid0.onReport([](auto& rpt) {
			debug_i("Report received, %u bytes", rpt.length);
			hid0.requestReport();
		});
		hid0.requestReport();
		return &hid0;
	});
#endif

#if CFG_TUH_MSC
	USB::MSC::onMount([](auto& inst) {
		debug_i("MSC mount %u", inst.dev_addr);
		msc0.begin(inst);
		msc0.enumerate([](USB::MSC::LogicalUnit& unit, USB::MSC::Inquiry inquiry) {
			Serial << unit << endl;
			for(auto part : unit.partitions()) {
				Serial << part << endl;
			}

			return true; // Continue enumerating
		});
		return &msc0;
	});
#endif

#if CFG_TUH_CDC
	USB::CDC::onMount([](auto& inst) {
		debug_i("CDC mount %u", inst.idx);
		cdc0.begin(inst);
		// cdc0.systemDebugOutput(true);
		cdc0.onDataReceived([](Stream& stream, char arrivedChar, unsigned short availableCharsCount) {
			char buf[availableCharsCount];
			auto n = stream.readBytes(buf, availableCharsCount);
			Serial.write(buf, n);
		});
		return &cdc0;
	});
	USB::CDC::onUnmount([](USB::CDC::HostDevice& dev) { dev.systemDebugOutput(false); });

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

	timer.initializeMs<3000>(InterruptCallback([]() { debug_i("Alive"); }));
	timer.start();
}
