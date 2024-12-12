# esp32sx support not yet included in tinyusb
COMPONENT_SOC := rp2* esp32s2 esp32s3 host

COMPONENT_DEPENDS := USB
DISABLE_NETWORK := 1

USB_CONFIG := basic_host.usbcfg
