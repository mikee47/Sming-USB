#pragma once

#include <tusb.h>
#include <Delegate.h>
#include <usb_classdefs.h>

namespace USB
{
using GetDeviceDescriptor = Delegate<const tusb_desc_device_t*(const tusb_desc_device_t& desc)>;

/**
 * @brief Structure of a USB string descriptor
*/
struct StringDescriptor {
	uint8_t length;   ///< Total size (in bytes) including this header
	uint8_t type;	 ///< Must be TUSB_DESC_STRING
	uint16_t value[]; ///< UTF16-LE encoded text (no NUL terminator)
};

/**
 * @brief Application-provided callback to customise string responses
 * @param index String index to fetch (STRING_INDEX_xxxx)
 * @retval const StringDescriptor* Pointer to persistent buffer containing descriptor.
 *         Return nullptr to use default value from string table.
 * @note Returned descriptor MUST NOT be on the stack! Typically this is statically allocated.
 */
using GetDescriptorString = Delegate<const StringDescriptor*(uint8_t index)>;

void onGetDeviceDescriptor(GetDeviceDescriptor callback);
void onGetDescriptorSting(GetDescriptorString callback);

} // namespace USB
