# esp32sx support not yet included in tinyusb
COMPONENT_SOC := rp2040

COMPONENT_DEPENDS := USB
DISABLE_NETWORK := 1

USB_CONFIG := basic_host.json
