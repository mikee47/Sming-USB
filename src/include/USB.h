#pragma once

#include <tusb.h>

#ifdef ENABLE_USB_CLASSES
#include <usb_classdefs.h>
#endif

namespace USB
{
bool begin();

} // namespace USB
