#include <SmingCore.h>
#include <USB.h>
#include <Storage/SpiFlash.h>
#include "xbox.h"

namespace
{
#ifdef ARCH_HOST

#define DESC_TYPE_MAP(XX)                                                                                              \
	XX(DEVICE, 0x01)                                                                                                   \
	XX(CONFIGURATION, 0x02)                                                                                            \
	XX(STRING, 0x03)                                                                                                   \
	XX(INTERFACE, 0x04)                                                                                                \
	XX(ENDPOINT, 0x05)                                                                                                 \
	XX(DEVICE_QUALIFIER, 0x06)                                                                                         \
	XX(OTHER_SPEED_CONFIG, 0x07)                                                                                       \
	XX(INTERFACE_POWER, 0x08)                                                                                          \
	XX(OTG, 0x09)                                                                                                      \
	XX(DEBUG, 0x0A)                                                                                                    \
	XX(INTERFACE_ASSOCIATION, 0x0B)                                                                                    \
	XX(BOS, 0x0F)                                                                                                      \
	XX(DEVICE_CAPABILITY, 0x10)                                                                                        \
	XX(CS_DEVICE, 0x21)                                                                                                \
	XX(CS_CONFIGURATION, 0x22)                                                                                         \
	XX(CS_STRING, 0x23)                                                                                                \
	XX(CS_INTERFACE, 0x24)                                                                                             \
	XX(CS_ENDPOINT, 0x25)                                                                                              \
	XX(SUPERSPEED_ENDPOINT_COMPANION, 0x30)                                                                            \
	XX(SUPERSPEED_ISO_ENDPOINT_COMPANION, 0x31)

const char* getDescTypeName(uint8_t type)
{
	switch(type) {
#define XX(name, value)                                                                                                \
	case value:                                                                                                        \
		return #name;
		DESC_TYPE_MAP(XX)
#undef XX
	}
	switch(USB::Descriptor::Type{type}.type) {
	case TUSB_REQ_TYPE_STANDARD:
		return "STANDARD";
	case TUSB_REQ_TYPE_CLASS:
		return "CLASS";
	case TUSB_REQ_TYPE_VENDOR:
		return "VENDOR";
	case TUSB_REQ_TYPE_INVALID:
		return "RESERVED";
	}
}

const char* getXferTypeName(uint8_t type)
{
	switch(type) {
	case TUSB_XFER_CONTROL:
		return "CTRL";
	case TUSB_XFER_ISOCHRONOUS:
		return "ISO";
	case TUSB_XFER_BULK:
		return "BULK";
	case TUSB_XFER_INTERRUPT:
		return "INT";
	default:
		return "?";
	}
}

const uint8_t xbox360_desc_data[]{
	0x09, 0x02, 0x99, 0x00, 0x04, 0x01, 0x00, 0xA0, 0xFA, 0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0x5D, 0x01,
	0x00, 0x11, 0x21, 0x00, 0x01, 0x01, 0x25, 0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x13, 0x01, 0x08, 0x00,
	0x00, 0x07, 0x05, 0x81, 0x03, 0x20, 0x00, 0x04, 0x07, 0x05, 0x01, 0x03, 0x20, 0x00, 0x08, 0x09, 0x04,
	0x01, 0x00, 0x04, 0xFF, 0x5D, 0x03, 0x00, 0x1B, 0x21, 0x00, 0x01, 0x01, 0x01, 0x82, 0x40, 0x01, 0x02,
	0x20, 0x16, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x82, 0x03, 0x20, 0x00, 0x02, 0x07, 0x05, 0x02, 0x03, 0x20, 0x00, 0x04, 0x07, 0x05, 0x83,
	0x03, 0x20, 0x00, 0x40, 0x07, 0x05, 0x03, 0x03, 0x20, 0x00, 0x10, 0x09, 0x04, 0x02, 0x00, 0x01, 0xFF,
	0x5D, 0x02, 0x00, 0x09, 0x21, 0x00, 0x01, 0x01, 0x22, 0x84, 0x07, 0x00, 0x07, 0x05, 0x84, 0x03, 0x20,
	0x00, 0x10, 0x09, 0x04, 0x03, 0x00, 0x00, 0xFF, 0xFD, 0x13, 0x04, 0x06, 0x41, 0x00, 0x01, 0x01, 0x03};

void parse_xbox360()
{
	using namespace USB;
	DescriptorList list{reinterpret_cast<const Descriptor*>(xbox360_desc_data), ARRAY_SIZE(xbox360_desc_data)};

	DescriptorEnum e(list);
	while(auto desc = e.next()) {
		// Serial << "DESC " << desc->type.value << ' ' << ", length " << desc->length << endl;
		m_printHex(getDescTypeName(desc->type), desc, desc->length, -1, 32);
		switch(desc->type) {
		case TUSB_DESC_INTERFACE: {
			auto itf = desc->as<tusb_desc_interface_t>();
			Serial << "  #" << itf->bInterfaceNumber << ", class " << itf->bInterfaceClass << ", subclass "
				   << itf->bInterfaceSubClass << ", protocol " << itf->bInterfaceProtocol << endl;
			break;
		}

		case TUSB_DESC_ENDPOINT: {
			auto ep = desc->as<tusb_desc_endpoint_t>();
			Serial << "  Addr " << String(ep->bEndpointAddress, HEX, 2) << ", xfer " << ep->bmAttributes.xfer << ' '
				   << getXferTypeName(ep->bmAttributes.xfer) << ", sync " << ep->bmAttributes.sync << ", usage "
				   << ep->bmAttributes.usage << ", MaxPacketSize " << ep->wMaxPacketSize << ", interval "
				   << ep->bInterval << endl;
		}

		default:;
		}
	}
}
#endif

Xbox xbox;

} // namespace

void tuh_mount_cb(uint8_t dev_addr)
{
	uint16_t vid{};
	uint16_t pid{};
	tuh_vid_pid_get(dev_addr, &vid, &pid);
	debug_i("A device with address %u is mounted, %04x:%04x", dev_addr, vid, pid);

	// XBOX 360 controller
	xbox.init(dev_addr);
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

		// By default host stack will use activate boot protocol on supported interface.
		// Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
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

#ifdef ARCH_HOST
	parse_xbox360();
#endif

	timer.initializeMs<3000>(InterruptCallback([]() { debug_i("Alive"); }));
	timer.start();
}
