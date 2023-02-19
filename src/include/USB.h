#pragma once

#include <tusb.h>

#ifdef ENABLE_USB_CLASSES

#if CFG_TUD_CDC
#include "USB/CDC/Serial.h"
#endif

#if CFG_TUD_MSC
#include "USB/MSC/Device.h"
#endif

#if CFG_TUH_MSC
#include "USB/MSC/HostDevice.h"
#endif

#include <usb_classdefs.h>

#endif

namespace USB
{
bool begin();

} // namespace USB
