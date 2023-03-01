#pragma once

#include "Descriptors.h"

namespace USB
{
class HostInterface
{
public:
	struct Instance {
		uint8_t dev_addr{255};
		uint8_t idx{255};
		const char* name;

		bool operator==(const Instance& other) const
		{
			return dev_addr == other.dev_addr && idx == other.idx;
		}
	};

	void begin(const Instance& inst)
	{
		this->inst = inst;
	}

	void end()
	{
		inst.dev_addr = 255;
		inst.idx = 255;
	}

	const char* getName() const
	{
		return inst.name;
	}

	uint8_t getAddress() const
	{
		return inst.dev_addr;
	}

	bool operator==(const HostInterface& other) const
	{
		return inst == other.inst;
	}

protected:
	Instance inst;
};

} // namespace USB
