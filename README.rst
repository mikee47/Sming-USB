USB
===

Sming library for TinyUSB.

At present a separate UsbStorage library implements Host functionality to access a USB storage device.
For device mode we'd also want to offer a storage device via USB.

TinyUSB currently offers good support for devices, with more limited Host support.
At present, there is no host support for Esp32 (only Rp2040).


Device stack
------------

The TinyUSB example applications generally consist of these three things:

tusb_config.h
    Contains definitions such as which classes are supported, how many endpoints (each),
    buffer sizes, etc.

usb_descriptors.c
    Provides descriptor structure definitions and implements required callbacks to provide
    these to the TinyUSB stack.

main.c
    Implements class-specific callbacks.

There's quite a bit of work involved in getting this right, even for standard classes,
and there is much that could be automated by a build script which translates the user
requirements (probably JSON) into the appropriate structures and code templates.

The tool would also validate the specification against the target hardware.

In addition, the JSON specification could be created/edited using a graphical tool,
much like was done for the Storage partition editor.

The library should also provide wrapper classes for inspecting and generating low-level
USB structures, handling byte-ordering and other such detail.

Finally, the user code would be written as C++ class implementations, with the library
taking care of the callback implementations.


Host stack
----------

Supported classes must be declared 


Notes
-----

TinyUSB requires various global callback functions to be implemented.
TinyUSB defines optional ones as WEAK to avoid calling them when not implemented.

However, if an application implements them then they must be 'undefined' first.
This should be handled by the library.
