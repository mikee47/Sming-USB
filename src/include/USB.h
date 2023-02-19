#pragma once

#include <tusb.h>

#if CFG_TUH_ENABLED

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
