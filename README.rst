USB
===

Sming library for TinyUSB.


Notes
-----

TinyUSB requires various global callback functions to be implemented.
TinyUSB defines optional ones as WEAK to avoid calling them when not implemented.

However, if an application implements them then they must be 'undefined' first.
This library will probably end up handling all these low-level callbacks and provide
a more robust C++ interface to the application.
