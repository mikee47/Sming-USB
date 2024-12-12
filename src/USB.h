/****
 * USB.h
 *
 * Copyright 2023 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the Sming USB Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 * @author: 2018 - Mikee47 <mike@sillyhouse.net>
 *
 ****/

#pragma once

#include <tusb.h>

#ifdef ENABLE_USB_CLASSES
#include "USB/Classes.h"
#endif

namespace USB
{
/**
 * @brief Initialise the USB stack
 * @param host Pass true to initialise for host mode, false for device mode
 *
 * OTG devices support operating as A (host) or B (device).
 * This function can be called to switch between the two.
 */
bool begin(bool host);

/**
 * @brief Stop USB operation
 */
void end();

} // namespace USB
