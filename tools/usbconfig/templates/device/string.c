//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
static const char* string_desc_arr[] =
{
  // 0: supported language is English (0x0409)
  "\x04\x03\x09\x04",
${string_data}
};

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
const uint16_t* tud_get_descriptor_string(uint8_t index)
{
  if(index >= TU_ARRAY_SIZE(string_desc_arr)) {
    return NULL;
  }

  return (const uint16_t*)string_desc_arr[index];
}
