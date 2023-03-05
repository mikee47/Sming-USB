#pragma once

#include <tusb.h>
#include <Delegate.h>
#include <WString.h>
#if CFG_TUD_ENABLED
#include <usb_descriptors.h>
#endif

namespace USB
{
using GetDeviceDescriptor = Delegate<const tusb_desc_device_t*(const tusb_desc_device_t& desc)>;

/**
 * @brief Structure of a USB descriptor
*/
struct Descriptor {
	union Type {
		uint8_t value;
		struct {
			uint8_t id : 5;
			uint8_t type : 2; // tusb_request_type_t
			uint8_t reserved : 1;
		};
	};

	static_assert(sizeof(Type) == 1, "Bad alignment");

	uint8_t length; ///< Total size (in bytes) including this header
	uint8_t type;   ///< e.g. TUSB_DESC_STRING
	// uint8_t content[];

	template <typename T> const T* as() const
	{
		return reinterpret_cast<const T*>(this);
	}
};

/**
 * @brief Buffer containing list of descriptors
 */
struct DescriptorList {
	const Descriptor* desc;
	size_t length;
};

class DescriptorEnum
{
public:
	DescriptorEnum(DescriptorList list) : list(list)
	{
	}

	void reset()
	{
		offset = 0;
	}

	const Descriptor* next()
	{
		if(offset >= list.length) {
			return nullptr;
		}

		auto ptr = reinterpret_cast<const uint8_t*>(list.desc);
		ptr += offset;
		auto desc = reinterpret_cast<const Descriptor*>(ptr);
		offset += desc->length;
		return (offset <= list.length) ? desc : nullptr;
	}

private:
	DescriptorList list;
	size_t offset{0};
};

/**
 * @brief Template for making a USB string descriptor
*/
template <size_t max_chars> struct StringDescriptor : public Descriptor {
	uint16_t text[max_chars]; ///< UTF16-LE encoded text (no NUL terminator)

	StringDescriptor() : Descriptor{2, TUSB_DESC_STRING}
	{
	}

	StringDescriptor(const char* str, size_t charCount) : StringDescriptor()
	{
		charCount = std::min(charCount, max_chars);
		for(unsigned i = 0; i < charCount; ++i) {
			text[i] = str[i];
		}
		length = 2 + (charCount * 2);
	}

	StringDescriptor(const String& s) : StringDescriptor(s.c_str(), s.length())
	{
	}
};

static_assert(sizeof(StringDescriptor<8>) == 18, "Bad descriptor alignment");

/**
 * @brief Application-provided callback to customise string responses
 * @param index String index to fetch (STRING_INDEX_xxxx)
 * @retval const StringDescriptor* Pointer to persistent buffer containing descriptor.
 *         Return nullptr to use default value from string table.
 * @note Returned descriptor MUST NOT be on the stack! Typically this is statically allocated.
 */
using GetDescriptorString = Delegate<const Descriptor*(uint8_t index)>;

void onGetDeviceDescriptor(GetDeviceDescriptor callback);
void onGetDescriptorSting(GetDescriptorString callback);

} // namespace USB
