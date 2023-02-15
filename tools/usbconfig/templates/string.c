//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors, first element is the string length
static const char* string_desc_arr [] =
{
  "\x02\x09\x04", // 0: is supported language is English (0x0409)
${strings}};

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void)langid;

  struct StringBuffer {
    uint8_t size; // Total size including header
    uint8_t type; // String type
    uint16_t str[${max_string_len}];
  };
  StringBuffer buf;
  uint8_t chr_count;

  if (index == 0)
  {
    memcpy(buf.str, string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if(index >= TU_ARRAY_SIZE(string_desc_arr)) {
      return NULL;
    }

    const char* str = string_desc_arr[index];

    // Convert ASCII string into UTF-16
    chr_count = *str++;
    for(unsigned i=0; i<chr_count; ++i) {
      buf.str[i] = str[i];
    }
  }

  buf.size = 2 + (chr_count * 2);
  buf.type = TUSB_DESC_STRING;

  return (const uint16_t*)&buf;
}
