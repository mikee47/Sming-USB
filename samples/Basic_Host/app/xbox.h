#pragma once

#include <USB.h>
#include <Data/BitSet.h>

// tag, type, bits, compare mask
#define XBOX360_INPUT_MAP(XX)                                                                                          \
	XX(dpad_up, bool, 1, 0x01)                                                                                         \
	XX(dpad_down, bool, 1, 0x01)                                                                                       \
	XX(dpad_left, bool, 1, 0x01)                                                                                       \
	XX(dpad_right, bool, 1, 0x01)                                                                                      \
	XX(btn_start, bool, 1, 0x01)                                                                                       \
	XX(btn_back, bool, 1, 0x01)                                                                                        \
	XX(btn_stick_left, bool, 1, 0x01)                                                                                  \
	XX(btn_stick_right, bool, 1, 0x01)                                                                                 \
	XX(btn_trig_left, bool, 1, 0x01)                                                                                   \
	XX(btn_trig_right, bool, 1, 0x01)                                                                                  \
	XX(btn_mode, bool, 1, 0x01)                                                                                        \
	XX(btn_unk1, bool, 1, 0x01)                                                                                        \
	XX(btn_a, bool, 1, 0x01)                                                                                           \
	XX(btn_b, bool, 1, 0x01)                                                                                           \
	XX(btn_x, bool, 1, 0x01)                                                                                           \
	XX(btn_y, bool, 1, 0x01)                                                                                           \
	XX(trig_left, uint8_t, 8, 0xff)                                                                                    \
	XX(trig_right, uint8_t, 8, 0xff)                                                                                   \
	XX(stick_left_x, int16_t, 16, 0xff00)                                                                              \
	XX(stick_left_y, int16_t, 16, 0xff00)                                                                              \
	XX(stick_right_x, int16_t, 16, 0xff00)                                                                             \
	XX(stick_right_y, int16_t, 16, 0xff00)

namespace USB::VENDOR
{
class Xbox : public HostDevice
{
public:
	enum class Input {
#define XX(tag, ...) tag,
		XBOX360_INPUT_MAP(XX)
#undef XX
			MAX
	};

	using InputMask = BitSet<uint32_t, Input>;
	using InputChange = Delegate<void(InputMask changed)>;

	struct InputData {
#define XX(tag, type, size, ...) type tag : size;
		XBOX360_INPUT_MAP(XX)
#undef XX

		int16_t operator[](Input input) const
		{
			switch(input) {
#define XX(tag, ...)                                                                                                   \
	case Input::tag:                                                                                                   \
		return tag;
				XBOX360_INPUT_MAP(XX)
#undef XX
			default:
				return 0;
			}
		}
	};

	enum class LedCommand {
		off,				   //   0: off
		all_blink_once,		   //   1: all blink, then previous setting
		top_left_blink_on,	 //   2: 1/top-left blink, then on
		top_right_blink_on,	//   3: 2/top-right blink, then on
		bottom_left_blink_on,  //   4: 3/bottom-left blink, then on
		bottom_right_blink_on, //   5: 4/bottom-right blink, then on
		top_left_on,		   //   6: 1/top-left on
		top_right_on,		   //   7: 2/top-right on
		bottom_left_on,		   //   8: 3/bottom-left on
		bottom_right_on,	   //   9: 4/bottom-right on
		rotate,				   //  10: rotate
		blink,				   //  11: blink, based on previous setting
		slow_blink,			   //  12: slow blink, based on previous setting
		rotate2,			   //  13: rotate with two lights
		slow_blink_persist,	//  14: persistent slow all blink
		blink_once,			   //  15: blink once, then previous setting
	};

	static bool probe(uint8_t dev_addr);
	bool begin(const Instance& inst, DescriptorEnum itf);
	void end() override;
	bool read();
	bool setled(LedCommand cmd);
	bool rumble(uint8_t strong, uint8_t weak);

	void onChange(InputChange callback)
	{
		inputChangeCallback = callback;
	}

	const InputData& inputs() const
	{
		return inputData;
	}

	static const char* getInputName(Xbox::Input input);

	bool setConfig(uint8_t itf_num) override;
	bool transferComplete(const Transfer& txfr) override;

private:
	bool control(tusb_request_recipient_t recipient, uint16_t value, uint16_t length);
	void control_cb(tuh_xfer_t* xfer);
	void process_packet();
	bool output(const void* data, uint8_t length);

	static void static_control_cb(tuh_xfer_t* xfer)
	{
		auto self = reinterpret_cast<Xbox*>(xfer->user_data);
		self->control_cb(xfer);
	}

	static constexpr size_t bufSize{64};
	InputData inputData{};
	InputChange inputChangeCallback;
	uint8_t buffer[bufSize];
	uint8_t output_buffer[8];
	uint8_t daddr;
	uint8_t ep_in{0x81};
	uint8_t ep_out{0x01};
	uint8_t state{0};
};

} // namespace USB::VENDOR
