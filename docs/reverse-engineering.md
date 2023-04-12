# reverse engineering

An initial search on the motor part number (BCM500DS-KZW) didn't turn up much apart from few hits on the BCM500DS-TYZ, which seems to be a Zigbee variant used by Zemismart.

Removing the end cap on the motor reveals the circuit board which is easily removed by sliding it out of the housing.

![alt text](images/board.jpg?raw=true?raw=true)

This looks like the board is the same for both variants, just with a different module populated for Zigbee and WiFi.

The board has 2 main components on it, namely a [PSF-BTA](https://github.com/CoolKit-Technologies/DevDocs/blob/master/WiFi/PSF-XTA%20%E5%BA%94%E7%94%A8%E6%8C%87%E5%AF%BC%E4%B9%A6.md) and a [UM8005](http://www.gztrchip.com/uploadfile/file/20210811/1628649277678726.pdf).

A little digging into what data I could find on these parts reveals that the UM8005 seems to handle the control duties and the PSF-BTA is basically an ESP8266 that acts like a bridge between the device and the eWeLink cloud, which is good news. We can Tasmostize it if we can just figure out the protocol.

## logic analyzer

I got out my logic analyzer and checking that the board is isolated so I don't plug live voltages into the USB port on my computer, I was able to hook up some nice thin mod wire to the board on the pins of interest according to the datasheet of the PSF-BTA.

![alt text](images/logic-analyzer.jpg?raw=true?raw=true)

I was capture some traces of the startup and of the serial comms.

![alt text](images/at-commands.jpg?raw=true?raw=true)

Looks like the chips communicate using AT commands with a baud rate of 19200. I tried lifting the pins of the UM8005 and using a serial terminal app to issue "on" and "off" commands to the UM8005, but it seemed to be ignoring me. There must be more to this.

Looking at the datasheet again, it looks like the startup sequence must be important somehow.

![alt text](images/startup.jpg?raw=true?raw=true)

The serial port seems to flip through a few different baud rates, with the output being mostly ESP bootloader information, but then settles on 19200. 

According the to the datasheet, the PSF-BTA STATUS pin indicates the status of the communication channel. The traces show the status pin shows No WiFi -> No Server -> Normal. Shortly after that, the PSF-BTA issues an AT+START[1B] command.

Also not shown in this trace is a software watchdog output that also seems to be emulated by pulling the line low for 100ms in a 10 second window.

# commands

## Updates from eWeLink Server

Using the eWeLink app to open the curtains:

```
[PSF-BTA] AT+UPDATE="sequence":"1680944101082","switch":"on"[1B]
[UM8005]  AT+RESULT="sequence":"1680944101082"[1B]
... curtain moves ...
[UM8005]  AT+UPDATE="sequence":"1680944101082","setclose":0[1B]
[PSF-BTA] AT+SEND=ok
```
Using the eWeLink app to close the curtains (ignoring sequence numbers):

```
[PSF-BTA] AT+UPDATE="sequence":"...","switch":"off"[1B]
[UM8005]  AT+RESULT="sequence":"..."[1B]
... curtain moves ...
[UM8005]  AT+UPDATE="sequence":"...","setclose":100[1B]
[PSF-BTA] AT+SEND=ok
```
Using the eWeLink app to stop the curtains when moving (ignoring sequence numbers):

```
[PSF-BTA] AT+UPDATE="sequence":"...","switch":"off"[1B]
[UM8005]  AT+RESULT="sequence":"..."[1B]
... curtain moves ...
[PSF-BTA] AT+UPDATE="sequence":"...","switch":"pause"[1B]
[UM8005]  AT+RESULT="sequence":"..."[1B]
... curtain stops ...
[UM8005]  AT+UPDATE="sequence":"...","setclose":35[1B]
[PSF-BTA] AT+SEND=ok
```
Using the eWeLink app to set the curtain position (ignoring sequence numbers):

```
[PSF-BTA] AT+UPDATE="sequence":"...","setclose":50[1B]
[UM8005]  AT+RESULT="sequence":"..."[1B]
... curtain moves ...
[UM8005]  AT+UPDATE="sequence":"...","setclose":50[1B]
[PSF-BTA] AT+SEND=ok
```

## manual updates

Using the remote control or tugging on the curtain to get it to open or close only results in the final resting position being sent to the PSF-BTA (ignoring sequence numbers):

```
... curtain moves ...
... curtain stops ...
[UM8005]  AT+UPDATE="sequence":"...","setclose":25[1B]
[PSF-BTA] AT+SEND=ok
```

# conclusion

We have all of the information we need to reverse engineer the communication between the device and the eWeLink server in order take local control of the curtain motor and connect it to Home Assistant.

1) The PSF-BTA only acts like a serial bridge, allowing the eWeLink servers to issue commands to the curtains, and for the curtains to send back position information.
2) We will need to emulate the watchdog pin.
3) The startup sequence is important as well. We need to emulate the status pin while we pretend to make a connection to the backend and then issue an AT+START[1B].
4) Then we can issue AT+UPDATE commands to set the position.
5) We need to send back an AT+SEND=ok when the motor tells us its position.
