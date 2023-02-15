enum
{
  ITF_NUM_HID,
  ITF_NUM_TOTAL
};

#define  CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_HID   0x81

const uint8_t desc_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

#if TUD_OPT_HIGH_SPEED
// Per USB specs: high speed capable device must report device_qualifier and other_speed_configuration

// device qualifier is mostly similar to device descriptor since we don't change configuration based on speed
static const tusb_desc_device_qualifier_t desc_device_qualifier =
{
  .bLength            = sizeof(tusb_desc_device_qualifier_t),
  .bDescriptorType    = TUSB_DESC_DEVICE_QUALIFIER,
  .bcdUSB             = USB_BCD,

  .bDeviceClass       = 0x00,
  .bDeviceSubClass    = 0x00,
  .bDeviceProtocol    = 0x00,

  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .bNumConfigurations = 0x01,
  .bReserved          = 0x00
};

// Invoked when received GET DEVICE QUALIFIER DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete.
// device_qualifier descriptor describes information about a high-speed capable device that would
// change if the device were operating at the other speed. If not highspeed capable stall this request.
const uint8_t* tud_descriptor_device_qualifier_cb(void)
{
  return (const uint8_t*)&desc_device_qualifier;
}

// Invoked when received GET OTHER SEED CONFIGURATION DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
// Configuration descriptor in the other speed e.g if high speed then this is for full speed and vice versa
const uint8_t* tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
  (void)index; // for multiple configurations

  // other speed config is basically configuration with type = OHER_SPEED_CONFIG
  static uint8_t desc_other_speed_config[CONFIG_TOTAL_LEN];
  memcpy(desc_other_speed_config, desc_configuration, CONFIG_TOTAL_LEN);
  desc_other_speed_config[1] = TUSB_DESC_OTHER_SPEED_CONFIG;

  // this example use the same configuration for both high and full speed mode
  return desc_other_speed_config;
}

#endif // highspeed

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
const uint8_t* tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index; // for multiple configurations

  // This example use the same configuration for both high and full speed mode
  return desc_configuration;
}
