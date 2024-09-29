#include "Arduino.h"
#include <RcSwitchReceiver.hpp>


constexpr int RX433_DATA_PIN = 6;
static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;

//The setup function is called once at startup of the sketch
void setup()
{
	Serial.begin(115200);
	rcSwitchReceiver.setup();
}

// The loop function is called in an endless loop
void loop()
{
	if (rcSwitchReceiver.available()) {
		const uint32_t value = rcSwitchReceiver.receivedValue();
		Serial.print("Received ");
		Serial.print(value);


		const size_t n = rcSwitchReceiver.receivedProtocolCount();
		Serial.print(" / Protocol number");
		if(n > 1) {
			Serial.print("s:");
		} else {
			Serial.print(':');
		}

		for(size_t i = 0; i < n; i++) {
			const int protocolNumber = rcSwitchReceiver.receivedProtocol(i);
			Serial.print(' ');
			Serial.print(protocolNumber);
		}

		Serial.println();

		rcSwitchReceiver.resetAvailable();
	}
}
