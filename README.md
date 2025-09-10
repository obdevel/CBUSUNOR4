<img align="right" src="arduino_cbus_logo.png"  width="150" height="75">

# Arduino library for MERG CBUS running over CAN bus

A library that implements the abstract CBUS base class. It supports the CAN peripheral on the Renesas microcontroller used by the UNO and Nano R4.

Note that this library depends on a number of other libraries which must also be downloaded and included in the sketch:

CBUS 			- abstract CBUS base class
UNOR4CAN		- driver for the CAN peripheral on the Renesas microcontroller
CBUSswitch		- CBUS switch
CBUSLED			- CBUS SLiM and FLiM LEDs
CBUSconfig		- CBUS module configuration
Streaming		- C++ style output

## Hardware

Tested on Arduino UNO R4 Minima and Nano R4

## Documentation

See the example 1-in-1-out sketch

## Limitations

CAN pins are fixed: CANTX = D4, CANRX = D5

## License

Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
