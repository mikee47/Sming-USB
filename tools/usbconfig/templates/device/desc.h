/*
 * usb_descriptors.h
 *
 * This file is auto-generated. Please do not edit!
 *
 */

#pragma once

enum UsbStringIndex {
  STRING_INDEX_LOCALES,
${string_ids}
  STRING_INDEX_COUNT
};

enum HidReportId {
  REPORT_ID_INVALID = 0,
${hid_report_ids}
  REPORT_ID_COUNT
};

enum DfuAlternateId {
${dfu_alternate_ids}
  DFU_ALTERNATE_COUNT
};
