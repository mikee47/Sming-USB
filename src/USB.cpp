/****
 * USB.cpp
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

#include "USB.h"
#include <Platform/System.h>

namespace
{
void poll()
{
#if CFG_TUD_ENABLED
	tud_task_ext(0, false);
#endif

#if CFG_TUH_ENABLED
	tuh_task_ext(0, false);
#endif

	System.queueCallback(poll);
}

} // namespace

namespace USB
{
bool begin()
{
	extern void initHardware();

	initHardware();

	bool res{true};

#if CFG_TUD_ENABLED
	res &= tud_init(BOARD_TUD_RHPORT);
#endif

#if CFG_TUH_ENABLED
	res &= tuh_init(BOARD_TUH_RHPORT);
#endif

	if(res) {
		poll();
	}

	return res;
}

} // namespace USB
