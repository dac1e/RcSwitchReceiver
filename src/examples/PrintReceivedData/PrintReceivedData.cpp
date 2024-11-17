
#include "RcSwitchReceiver.hpp"

#include "test/RcSwitch_test.hpp"

#include <Arduino.h>
#include "../../internal/ProtocolTimingSpec.hpp"

constexpr int RX433_DATA_PIN = 6;
static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;

// Reference to the serial to be used for printing.
UARTClass& serial = Serial3;

//The setup function is called once at startup of the sketch
void setup()
{
	serial.begin(9600);
	delay(100);
#if DEBUG_RCSWITCH_PROTOCOL_SPEC
	RcSwitch::printRxTimingTable(serial, 0);
	delay(300);
	RcSwitch::printRxTimingTable(serial, 1);
	delay(300);
#endif

#if ENABLE_RCSWITCH_TEST
	RcSwitch::RcSwitch_test::theTest.run();
#endif

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
