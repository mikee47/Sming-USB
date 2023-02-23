#include "USB.h"
#include <Platform/System.h>

#ifdef ARCH_ESP32
#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"
#include "hal/usb_ll.h"
#include "soc/periph_defs.h"
#include <driver/periph_ctrl.h>
#endif
namespace
{
#ifdef ARCH_ESP32

void configure_pins()
{
	for(auto iopin = usb_periph_iopins; iopin->pin >= 0; ++iopin) {
		if(iopin->ext_phy_only) {
			continue;
		}
		esp_rom_gpio_pad_select_gpio(iopin->pin);
		if(iopin->is_output) {
			esp_rom_gpio_connect_out_signal(iopin->pin, iopin->func, false, false);
		} else {
			esp_rom_gpio_connect_in_signal(iopin->pin, iopin->func, false);
			if((iopin->pin != GPIO_MATRIX_CONST_ZERO_INPUT) && (iopin->pin != GPIO_MATRIX_CONST_ONE_INPUT)) {
				gpio_ll_input_enable(&GPIO, gpio_num_t(iopin->pin));
			}
		}
		esp_rom_gpio_pad_unhold(iopin->pin);
	}
	gpio_ll_set_drive_capability(&GPIO, gpio_num_t(USBPHY_DM_NUM), GPIO_DRIVE_CAP_3);
	gpio_ll_set_drive_capability(&GPIO, gpio_num_t(USBPHY_DM_NUM), GPIO_DRIVE_CAP_3);
}

void initHardware()
{
	periph_module_reset(PERIPH_USB_MODULE);
	periph_module_enable(PERIPH_USB_MODULE);
	usb_ll_int_phy_enable();
	configure_pins();
}

#else

void initHardware()
{
}

#endif

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

USB::GetDeviceDescriptor deviceDescriptorCallback;
USB::GetDescriptorString descriptorStringCallback;

} // namespace

extern "C" {
const tusb_desc_device_t* tud_get_device_descriptor(void);
const uint16_t* tud_get_descriptor_string(uint8_t index);
}

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
const uint8_t* tud_descriptor_device_cb(void)
{
	auto desc = tud_get_device_descriptor();
	if(deviceDescriptorCallback) {
		desc = deviceDescriptorCallback(*desc);
	}
	return reinterpret_cast<const uint8_t*>(desc);
}

const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	(void)langid;
	auto str = tud_get_descriptor_string(index);
	if(descriptorStringCallback) {
		str = descriptorStringCallback(index);
	}

	return str;
}

namespace USB
{
bool begin()
{
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

void onGetDeviceDescriptor(GetDeviceDescriptor callback)
{
	deviceDescriptorCallback = callback;
}

void onGetDescriptorSting(GetDescriptorString callback)
{
	descriptorStringCallback = callback;
}

} // namespace USB
