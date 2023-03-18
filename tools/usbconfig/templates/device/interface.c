
//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
${itfnum_defs}
};

#define CONFIG_TOTAL_LEN (${config_total_len})

enum {
${epnum_defs}
};

static const uint8_t desc_configuration[] =
{
${config_desc}
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
