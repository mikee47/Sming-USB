//--------------------------------------------------------------------+
// HID Report Descriptors
//--------------------------------------------------------------------+

$report

const uint8_t* hid_reports[] = {$report_list};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
const uint8_t* tud_hid_descriptor_report_cb(uint8_t inst)
{
  return (inst < CFG_TUD_HID) ? hid_reports[inst] : NULL;
}
