#pragma once

#include <tusb.h>

#ifdef ENABLE_USB_CLASSES
#include "USB/Classes.h"
#endif

namespace USB
{
bool begin();
} // namespace USB
