//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

static const uint8_t desc_hid_report[] =
{
  TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(REPORT_ID_KEYBOARD         )),
  TUD_HID_REPORT_DESC_MOUSE   ( HID_REPORT_ID(REPORT_ID_MOUSE            )),
  TUD_HID_REPORT_DESC_CONSUMER( HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL )),
  TUD_HID_REPORT_DESC_GAMEPAD ( HID_REPORT_ID(REPORT_ID_GAMEPAD          ))
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
const uint8_t* tud_hid_descriptor_report_cb(uint8_t instance)
{
  (void)instance;
  return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

