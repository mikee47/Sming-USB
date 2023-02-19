#pragma once

#include <tusb.h>

#ifdef ENABLE_USB_CLASSES

#if CFG_TUD_MSC
#include "USB/MSC/Device.h"
#endif

#if CFG_TUH_MSC
#include "USB/MSC/HostDevice.h"
#endif

#if CFG_TUH_ENABLED
#include <usb_hostdefs.h>
#endif

#endif

namespace USB
{
bool begin();

} // namespace USB
