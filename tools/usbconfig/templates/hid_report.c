//--------------------------------------------------------------------+
// HID Report Descriptors
//--------------------------------------------------------------------+

${report}

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
const uint8_t* tud_hid_descriptor_report_cb(uint8_t instance)
{
  switch(instance) {
${callback}
  default: return NULL;
  }
}
