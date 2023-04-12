# flashing

The process to flash the PSF-BTA with Tasmota is complicated by the fact that the serial port is shared with the UM8005, so we need to hold that in a reset state while performing the flashing operation.

Also, the press-button doesn't connect to GPIO0, so we can't use that to get the ESP8266 into programming mode.

Here's what you'll have to do:

![alt text](images/board-programming.png?raw=true?raw=true)

1) To hold the UM8005 in reset, short out the RSTB pin to ground on P3. Connect your serial adapter to any available ground.
2) Solder some thin mod wire directly to pin 21 (RX) and pin 22 (TX) and connect to the opposing pins of your serial adapter. (RX->TX, TX->RX)
2) Solder another piece of thin mod wire onto pin 10 (GPIO0) so that we can use it to temporarily ground the pin on startup.

Here's my setup:

![alt text](images/programming.jpg?raw=true?raw=true)

Once you've compiled Tasmota with the driver, you should be able to hold the GPIO0 lead to ground while applying power. Once powered up, you can release it. Now we are should be in programming mode, so we're free to flash the device.

First, save the original firmware:

```
esptool.py -p /dev/ttyUSB0 read_flash 0 0x100000 eachen-original.bin
```

Then flash the new firmware:

```
esptool.py -p /dev/ttyUSB0 write_flash -fs 1MB -fm dout 0x0 tasmota.bin
```

Once flashing is successful, you can power the board down and disconnect all of wires before powering everything up again.

# configuration

You should be able to go through the usual process to get Tasmota onto your WiFi network by joining the hotspot and providing your WiFi SSID and password.

Then find the IP address and load up the web interface.

Go to Configuration -> Configure Module and select "Generic" from the drop-down list.
You'll have to save and wait for the device to restart. Then go back and set it up to look like this:

![alt text](images/module-config.png?raw=true?raw=true)

Make sure you save your changes, and after rebooting you should be greeted with:

![alt text](images/main.png?raw=true?raw=true)

Wiggling the slider should now move your curtains.