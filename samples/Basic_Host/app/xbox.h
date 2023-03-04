#pragma once

#include <USB.h>

class Xbox
{
public:
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

	bool init(uint8_t dev_addr);
	bool read();
	bool setled(LedCommand cmd);

private:
	void control(tusb_request_recipient_t recipient, uint16_t value, uint16_t length);
	void control_cb(tuh_xfer_t* xfer);

	static void static_control_cb(tuh_xfer_t* xfer)
	{
		auto self = reinterpret_cast<Xbox*>(xfer->user_data);
		self->control_cb(xfer);
	}

	static constexpr size_t bufSize{64};
	uint8_t buffer[bufSize];
	uint8_t daddr;
	uint8_t ep_in{0x81};
	uint8_t state{0};
};
