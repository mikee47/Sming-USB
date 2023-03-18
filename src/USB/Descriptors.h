/****
 * Descriptors.h
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

#include <tusb.h>
#include <Delegate.h>
#include <WString.h>
#include <Print.h>
#if CFG_TUD_ENABLED
#include <usb_descriptors.h>
#endif

namespace USB
{
/**
 * @brief Callback to support provision of dynamic device descriptors
 * @param desc The statically configured device descriptor
 * @param const tusb_desc_device_t* Application returns a pointer to a statically allocated descriptor to use
 *
 * Application typically copies the source descriptor to a statically allocated buffer, then amends
 * values as required.
 */
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

	/**
	 * @brief Less clumsy way to cast descriptor to a specific type
	 * @tparam T TinyUSB defines structures beginning with 'tusb_desc_'
	 */
	template <typename T> const T* as() const
	{
		return reinterpret_cast<const T*>(this);
	}

	size_t printTo(Print& p) const;
};

/**
 * @brief Buffer containing list of descriptors
 *
 * Supports C++ iteration.
 */
struct DescriptorList {
	const Descriptor* desc;
	size_t length;

	/**
	 * @name Iterator support (forward only)
	 * @{
	 */
	class Iterator
	{
	public:
		Iterator() = default;
		Iterator(const Iterator&) = default;

		Iterator(const DescriptorList* list, uint16_t offset) : mList(list), mOffset(offset)
		{
		}

		operator bool() const
		{
			return mList != nullptr && mOffset < mList->length;
		}

		bool operator==(const Iterator& rhs) const
		{
			return mList == rhs.mList && mOffset == rhs.mOffset;
		}

		bool operator!=(const Iterator& rhs) const
		{
			return !operator==(rhs);
		}

		const Descriptor* operator*() const
		{
			if(!*this) {
				return nullptr;
			}

			auto ptr = reinterpret_cast<const uint8_t*>(mList->desc);
			return reinterpret_cast<const Descriptor*>(ptr + mOffset);
		}

		Iterator& operator++()
		{
			next();
			return *this;
		}

		Iterator operator++(int)
		{
			Iterator tmp(*this);
			next();
			return tmp;
		}

		void next()
		{
			if(!*this) {
				return;
			}
			mOffset += operator*()->length;
		}

		using const_iterator = Iterator;

	private:
		const DescriptorList* mList{nullptr};
		uint16_t mOffset{0};
	};

	Iterator begin() const
	{
		return Iterator(this, 0);
	}

	Iterator end() const
	{
		return Iterator(this, length);
	}

	/** @} */

	size_t printTo(Print& p) const;
};

/**
 * @brief Template for making a USB string descriptor
*/
template <size_t max_chars> struct StringDescriptor : public Descriptor {
	uint16_t text[max_chars]; ///< UTF16-LE encoded text (no NUL terminator)

	/**
	 * @brief Construct an empty string descriptor
	 */
	StringDescriptor() : Descriptor{2, TUSB_DESC_STRING}
	{
	}

	/**
	 * @brief Construct a string descriptor containing text
	 * @param str ASCII text (unicode page #0 only)
	 * @param charCount Number of characters in string
	 */
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
