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
ifndef USB_CONFIG
$(error USB_CONFIG not defined)
endif

USBCONFIG_TOOL := $(PYTHON) $(COMPONENT_PATH)/tools/usbconfig/usbconfig.py

USB_OUTPUT_DIR := $(PROJECT_DIR)/$(BUILD_BASE)/USB
USB_OUTPUT_FILES := $(addprefix $(USB_OUTPUT_DIR)/,tusb_config.h usb_descriptors.h usb_descriptors.c)

COMPONENT_PREREQUISITES := $(USB_OUTPUT_FILES)

$(USB_OUTPUT_FILES): $(USB_CONFIG)
	$(USBCONFIG_TOOL) $(USB_CONFIG) $(USB_OUTPUT_DIR)

COMPONENT_APPCODE += $(USB_OUTPUT_DIR)
COMPONENT_INCDIRS += $(USB_OUTPUT_DIR)

GLOBAL_CFLAGS += -DCFG_TUSB_MCU=$(CFG_TUSB_MCU)

COMPONENT_VARS += USB_DEBUG_LEVEL
USB_DEBUG_LEVEL ?= 0
GLOBAL_CFLAGS += -DCFG_TUSB_DEBUG=$(USB_DEBUG_LEVEL)

COMPONENT_VARS += USB_DEVICE_CLASSES USB_HOST_CLASSES

ifdef USB_DEVICE_CLASSES

TINYUSB_DEVICE_CLASSES := \
	HID \
	CDC \
	MSC \
	MIDI \
	VENDOR

BAD_USB_DEVICE_CLASSES := $(filter-out $(TINYUSB_DEVICE_CLASSES),$(USB_DEVICE_CLASSES))
ifdef BAD_USB_DEVICE_CLASSES
$(warning Unknown USB device classes: $(BAD_USB_DEVICE_CLASSES))
$(error Valid classes are: $(TINYUSB_DEVICE_CLASSES))
endif
GLOBAL_CFLAGS += \
	-DUSB_DEVICE_CLASSES="$(USB_DEVICE_CLASSES)" \
	$(foreach c,$(USB_DEVICE_CLASSES),-DCFG_TUD_$c=1)

endif # USB_DEVICE_CLASSES

ifdef USB_HOST_CLASSES

TINYUSB_HOST_CLASSES := \
	HUB \
	HID \
	CDC \
	MSC \
	MIDI \
	VENDOR

BAD_USB_HOST_CLASSES := $(filter-out $(TINYUSB_HOST_CLASSES),$(USB_HOST_CLASSES))
ifdef BAD_USB_HOST_CLASSES
$(warning Unknown USB host classes: $(BAD_USB_HOST_CLASSES))
$(error Valid classes are: $(TINYUSB_HOST_CLASSES))
endif
GLOBAL_CFLAGS += \
	-DCFG_TUH_ENABLED=1 \
	-DUSB_HOST_CLASSES="$(USB_HOST_CLASSES)" \
	$(foreach c,$(USB_HOST_CLASSES),-DCFG_TUH_$c=1)

endif # USB_HOST_CLASSES


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
	tinyusb/src \
	tinyusb/src/portable/$(TUSB_FAMILY_PATH) \
	$(addprefix tinyusb/src/,$(TINYUSB_SRCDIRS))

COMPONENT_INCDIRS += \
	src/include \
	tinyusb/src

endif
