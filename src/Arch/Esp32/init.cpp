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

#include <USB.h>
#include <esp_err.h>
#include <esp_private/usb_phy.h>

namespace USB
{
usb_phy_handle_t phy_handle;

bool initHardware(bool host)
{
	if(phy_handle) {
		usb_del_phy(phy_handle);
		phy_handle = nullptr;
	}

	usb_phy_config_t phy_conf = {
		.controller = USB_PHY_CTRL_OTG,
		.target = USB_PHY_TARGET_INT,
		.otg_mode = host ? USB_OTG_MODE_HOST : USB_OTG_MODE_DEVICE,
		.otg_speed = host ? USB_PHY_SPEED_UNDEFINED : USB_PHY_SPEED_FULL,
	};

	auto err = usb_new_phy(&phy_conf, &phy_handle);
	return err == ESP_OK;
}

} // namespace USB
