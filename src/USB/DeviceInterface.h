/****
 * DeviceInterface.h
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
 ****/

#pragma once

#include "Descriptors.h"

namespace USB
{
/**
 * @brief Base class to support a USB device interface implementation
 */
class DeviceInterface
{
public:
	/**
	 * @brief Constructor
	 * @param instance TinyUSB instance or index (class-specific)
	 * @param name Declared name for this interface instance
	 */
	DeviceInterface(uint8_t instance, const char* name) : inst(instance), name(name)
	{
	}

	const char* getName() const
	{
		return name;
	}

protected:
	uint8_t inst;
	const char* name;
};

} // namespace USB
