Basic Device
============

Application demonstrating how to set up USB device with multiple interfaces.

The USB configuration is described in ``basic_device.usbcfg``, and the corresponding
TinyUSB configuration files generated in, for example, ``out/Rp2040/debug/USB``.

C++ classes are provided for some of the standard interfaces


The MIDI device can be tested using the linux ``aplaymidi`` utility.
Type ``aplaymidi -l`` to show available devices.
Play a MIDI file with ``aplaymidi -p 24:0 test.mid``.

.. note::

    This sample works as-is for the rp2040.
    The esp32s2 has limited endpoints so some of the interfaces need to be commented-out in ``basic_device.usbcfg``.
    Perhaps try starting with just ``midi`` interface.
