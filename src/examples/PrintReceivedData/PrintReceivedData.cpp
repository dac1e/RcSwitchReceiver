#include "Arduino.h"
#include <RcSwitchReceiver.hpp>


constexpr int RX433_DATA_PIN = 6;
static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;

// Reference to the serial to be used for printing.
UARTClass& serial = Serial;

//The setup function is called once at startup of the sketch
void setup()
{
	serial.begin(9600);
	rcSwitchReceiver.begin();
}

// The loop function is called in an endless loop
void loop()
{
	if (rcSwitchReceiver.available()) {
		const uint32_t value = rcSwitchReceiver.receivedValue();
		serial.print("Received ");
		serial.print(value);


		const size_t n = rcSwitchReceiver.receivedProtocolCount();
		serial.print(" / Protocol number");
		if(n > 1) {
			serial.print("s:");
		} else {
			serial.print(':');
		}

		for(size_t i = 0; i < n; i++) {
			const int protocolNumber = rcSwitchReceiver.receivedProtocol(i);
			serial.print(' ');
			serial.print(protocolNumber);
		}

		serial.println();

		rcSwitchReceiver.resetAvailable();
	}
}
