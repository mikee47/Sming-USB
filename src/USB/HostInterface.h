/****
 * HostInterface.h
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
 * @brief Common base class to support Host USB access
 */
class HostInterface
{
public:
	/**
	 * @brief Identifies a TinyUSB host interface
	 */
	struct Instance {
		uint8_t dev_addr{255}; ///< Device address (from 1)
		uint8_t idx{255};	  ///< Index or instance value specific to class
		const char* name;	  ///< Optional name for this interface instance

		bool operator==(const Instance& other) const
		{
			return dev_addr == other.dev_addr && idx == other.idx;
		}
	};

	/**
	 * @brief Descendant classes should override this method to peform initialisation
	 */
	void begin(const Instance& inst)
	{
		this->inst = inst;
	}

	/**
	 * @brief Called when device is disconnected. Override as required.
	 */
	virtual void end()
	{
		inst.dev_addr = 255;
		inst.idx = 255;
	}

	const char* getName() const
	{
		return inst.name;
	}

	uint8_t getAddress() const
	{
		return inst.dev_addr;
	}

	bool operator==(const HostInterface& other) const
	{
		return inst == other.inst;
	}

	bool operator==(const Instance& other) const
	{
		return inst == other;
	}

protected:
	Instance inst;
};

} // namespace USB
