COMPONENT_SOC := \
	rp2040 \
	esp32s2 \
	esp32s3

COMPONENT_SUBMODULES := tinyusb
COMPONENT_LIBNAME :=

ifeq ($(SMING_ARCH),Rp2040)
TUSB_FAMILY_PATH := raspberrypi/rp2040
CFG_TUSB_MCU := OPT_MCU_RP2040
else ifeq ($(SMING_SOC),esp32s2)
TUSB_FAMILY_PATH := espressif/esp32sx
CFG_TUSB_MCU := OPT_MCU_ESP32S2
else ifeq ($(SMING_SOC),esp32s3)
TUSB_FAMILY_PATH := espressif/esp32sx
CFG_TUSB_MCU := OPT_MCU_ESP32S3
else
TUSB_FAMILY_PATH :=
endif

ifdef TUSB_FAMILY_PATH

COMPONENT_VARS += USB_CONFIG
ifdef USB_CONFIG

USBCONFIG_TOOL := $(PYTHON) $(COMPONENT_PATH)/tools/usbconfig/usbconfig.py

USB_OUTPUT_DIR := $(PROJECT_DIR)/$(BUILD_BASE)/USB
USB_CONFIG_H := $(USB_OUTPUT_DIR)/tusb_config.h

COMPONENT_PREREQUISITES := $(USB_CONFIG_H)

$(USB_CONFIG_H): $(USB_CONFIG)
	$(USBCONFIG_TOOL) $(USB_CONFIG) $(USB_OUTPUT_DIR)

endif # USB_CONFIG

COMPONENT_APPCODE += $(USB_OUTPUT_DIR)
COMPONENT_INCDIRS += $(USB_OUTPUT_DIR)

GLOBAL_CFLAGS += -DCFG_TUSB_MCU=$(CFG_TUSB_MCU)

COMPONENT_VARS += USB_DEBUG_LEVEL
USB_DEBUG_LEVEL ?= 0
GLOBAL_CFLAGS += -DCFG_TUSB_DEBUG=$(USB_DEBUG_LEVEL)

GLOBAL_CFLAGS += \
	-DCFG_TUSB_DEBUG_PRINTF=m_printf

TINYUSB_SRCDIRS := \
	common \
	device \
	class/audio \
	class/cdc \
	class/dfu \
	class/hid \
	class/midi \
	class/msc \
	class/net \
	class/usbtmc \
	class/video \
	class/vendor \
	host

COMPONENT_APPCODE += \
	src \
	src/CDC \
	src/HID \
	src/MIDI \
	src/MSC \
	tinyusb/src \
	tinyusb/src/portable/$(TUSB_FAMILY_PATH) \
	$(addprefix tinyusb/src/,$(TINYUSB_SRCDIRS))

COMPONENT_INCDIRS += \
	src/include \
	tinyusb/src

endif
