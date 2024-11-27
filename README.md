# RcSwitchReceiver
433Mhz / 315 Mhz remote control receiver and decoder.

## Description:
The library receives and decodes the pulses received by a remote control transmitter. It focuses on minimizing the interrupt
handler runtime. This is achieved by doing many timing calculations already at compile time rather than at runtime. 
Multiple instances of the API class can operate in parallel on different IO pins.
Another feature is, that pulses received from a remote control transmitter can be analyzed by dumping them.

## Hints on remote operating distance:
The development was done on an Arduino Due. Tests have shown that the operating distance of several 433Mhz receiver modules 
strongly depend on the quality of the power supply. When the Arduino Due is supplied via USB port, the receiver module works 
properly when powered from the Arduino 5V pin. When the Arduino Due is supplied via the extra power connector, the Arduino 
5V pin is is powered from an Arduino Due internal voltage regulator. A receiver module now powerd from the Arduino 
Due 5V pin, will drop the operating distance by at least 50% compared to the USB powering situation. So when the extra power 
connector is used for Arduino Due, I recommend suppying the receiver module separately by a linear Voltage regulator like an 
7805, to achieve best operating distance.
