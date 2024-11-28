# RcSwitchReceiver
433Mhz / 315 Mhz remote control receiver and decoder.

## Description:
The library decodes the pulses received by a remote control transmitter. It focuses on minimizing the interrupt
handler runtime. Multiple instances of the receiver can operate in parallel on different IO pins.
Another feature is, that pulses received from a remote control transmitter can be dumped.

## Hints on remote operating distance:
The development was done on an Arduino Due. Tests have shown that the operating distance of several 433Mhz receiver modules 
strongly depend on the quality of the power supply. When the Arduino Due is supplied via USB port, the receiver module works 
properly when powered from the Arduino 5V pin. When the Arduino Due is supplied via the extra power connector, the Arduino 
5V pin is is powered from an Arduino Due internal voltage regulator. A receiver module now powerd from the Arduino 
Due 5V pin, will drop the operating distance by at least 50% compared to the USB powering situation. So when the extra power 
connector is used for Arduino Due, I recommend suppying the receiver module separately by a linear Voltage regulator like an 
7805, to achieve best operating distance.

## Remote control protocol
The remote control protocol is a stream of pulse pairs with different duration and
pulse levels. In the context of this documentation, the first pulse will be
referred to as "pulse A" and the second one as "pulse B".

```
  Normal level protocols start with a high level:
         ___________________
    XXXX|                   |____________________|XXXX


  Inverse level protocols start with a low level:
                             ____________________
    XXXX|___________________|                    |XXXX

        ^                   ^                    ^
        | pulse A duration  | pulse B duration   |
```
```
 In the synchronization phase there is a short pulse followed by a very long pulse:
    Normal level protocols:
         ____
    XXXX|    |_____________________________________________________________|XXXX


    Inverse level protocols:
              _____________________________________________________________
    XXXX|____|                                                             |XXXX
```


 In the data phase there is
  a short pulse followed by a long pulse for a logical 0 data bit:
```
    Normal level protocols:
         __
    XXXX|  |________|XXXX


    Inverse level protocols:
            ________
    XXXX|__|        |XXXX
```

  a long pulse followed by a short pulse for a logical 1 data bit:
```
    Normal level protocols:
         ________
    XXXX|        |__|XXXX


    Inverse level protocols:
                  __
    XXXX|________|  |XXXX

```
