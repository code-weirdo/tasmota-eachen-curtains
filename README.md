# tasmota-eachen-curtains

A driver for Tasmota that allows driving of EACHEN WiFi Smart Motorized Curtain Set, allowing control of your curtains over MQTT and/or Home Assistant.

![alt text](docs/images/main.png?raw=true?raw=true)

The driver is loosly based on the PS16DZ driver in the Tasmota code base, which I found also emulated the AT commands used by the eWeLink backend, but additional emulation of the link status and watchdog output pins needed to be added to get it all working.

# diclaimer

:warning: **DANGER OF ELECTROCUTION** :warning:

As with any electronic device that connects to mains power, there is a danger of electrocution if safety precautions are not followed and if you don't know what you're doing, please don't attempt this at home.

I take responsibility or liability for using the software or the instructions, tips, advice, etc. provided here!

# docs

The following documentation details how to compile a custom Tasmota image with the driver and how to flash it onto the device:

- [Reverse engineering the curtain motor](docs/reverse-engineering.md)
- [Compiling Tasmota](docs/compiling-tasmota.md)
- [Flashing Tasmota onto the curtain motor](docs/flashing.md)
