/*
 * tusb_config.h
 *
 * This file is auto-generated. Please do not edit!
 *
 */

#pragma once

// Use C++ support classes
#define ENABLE_USB_CLASSES 1

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif

#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_ENABLED ${device_enabled}

// Device interface classes
${device_classes}

// Device defines
${device_globals}

//--------------------------------------------------------------------
// HOST CONFIGURATION
//--------------------------------------------------------------------

#ifndef BOARD_TUH_RHPORT
#define BOARD_TUH_RHPORT      0
#endif

#define CFG_TUH_ENABLED ${host_enabled}

// Host interface classes
${host_classes}

// Host defines
${host_globals}

// max device support (excluding hub device)
#define CFG_TUH_DEVICE_MAX (CFG_TUH_HUB ? CFG_TUH_HUB_PORT_COUNT : 1)
