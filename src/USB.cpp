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
volatile bool taskQueued;

void poll()
{
	taskQueued = false;

#if CFG_TUD_ENABLED
	tud_task_ext(0, false);
#endif

#if CFG_TUH_ENABLED
	tuh_task_ext(0, false);
#endif
}

} // namespace

void tusb_time_delay_ms_api(uint32_t ms)
{
	os_delay_us(ms * 1000U);
}

#if CFG_TUD_ENABLED
void IRAM_ATTR tud_event_hook_cb(uint8_t, uint32_t, bool)
{
	if(!taskQueued) {
		System.queueCallback(poll);
		taskQueued = true;
	}
}
#endif

#if CFG_TUH_ENABLED
void IRAM_ATTR tuh_event_hook_cb(uint8_t, uint32_t, bool)
{
	if(!taskQueued) {
		System.queueCallback(poll);
		taskQueued = true;
	}
}
#endif

namespace USB
{
extern bool initHardware(bool host);

bool begin(bool host)
{
#if CFG_TUH_ENABLED
	if(host && tuh_inited()) {
		return true;
	}
#endif

#if CFG_TUD_ENABLED
	if(!host && tud_inited()) {
		return true;
	}
#endif

	end();

	if(!initHardware(host)) {
		return false;
	}

	const tusb_rhport_init_t rh_init = {
		.role = host ? TUSB_ROLE_HOST : TUSB_ROLE_DEVICE,
		.speed = TUSB_SPEED_AUTO,
	};
#if CFG_TUH_ENABLED
	if(host) {
		return tuh_rhport_init(BOARD_TUH_RHPORT, &rh_init);
	}
#endif

#if CFG_TUD_ENABLED
	if(!host) {
		return tud_rhport_init(BOARD_TUD_RHPORT, &rh_init);
	}
#endif

	debug_e("[USB] CFG_TU%c_ENABLED not defined", host ? 'H' : 'D');
	return false;
}

void end()
{
#if CFG_TUH_ENABLED
	if(tuh_inited()) {
		tuh_deinit(BOARD_TUH_RHPORT);
	}
#endif

#if CFG_TUD_ENABLED
	if(tud_inited()) {
		tud_deinit(BOARD_TUD_RHPORT);
	}
#endif
}

} // namespace USB
