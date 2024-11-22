

#include "RcSwitchReceiver.hpp"
// #include "test/RcSwitch_test.hpp"
#include <Arduino.h>

static const RxProtocolTable <
//                           #,  %,  clk,  syA,syB,  d0A,d0B,  d1A,d1B , inverseLevel
	makeProtocolTimingSpec<  1, 20,  350,  1,   31,    1,  3,    3,  1>, 		// ()
	makeProtocolTimingSpec<  2, 20,  650,  1,   10,    1,  3,    3,  1>, 		// ()
	makeProtocolTimingSpec<  3, 20,  100, 30,   71,    4, 11,    9,  6>, 		// ()
	makeProtocolTimingSpec<  4, 20,  380,  1,    6,    1,  3,    3,  1>, 		// ()
	makeProtocolTimingSpec<  5, 20,  500,  6,   14,    1,  2,    2,  1>, 		// ()
	makeProtocolTimingSpec<  6, 20,  450,  1,   23,    1,  2,    2,  1, true>, 	// (HT6P20B)
	makeProtocolTimingSpec<  7, 20,  150,  2,   62,    1,  6,    6,  1>, 		// (HS2303-PT)
	makeProtocolTimingSpec<  8, 20,  200,  3,  130,    7, 16,    3, 16>, 		// (Conrad RS-200 RX)

	makeProtocolTimingSpec< 10, 20,  365,  1,   18,    3,  1,    1,  3, true>, 	// (1ByOne Doorbell)
	makeProtocolTimingSpec< 11, 20,  270,  1,   36,    1,  2,    2,  1, true>, 	// (HT12E)
	makeProtocolTimingSpec< 12, 20,  320,  1,   36,    1,  2,    2,  1, true>  	// (SM5212)
> rxProtocolTable;

constexpr int RX433_DATA_PIN = 6;
static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;

// Reference to the serial to be used for printing.
UARTClass& serial = Serial3;

//The setup function is called once at startup of the sketch
void setup()
{
	serial.begin(9600);
	delay(100);
	serial.println();
	rcSwitchReceiver.begin(rxProtocolTable);

#if DEBUG_RCSWITCH_PROTOCOL_SPEC
	rcSwitchReceiver.dumpRxTimingTable(serial);
	delay(300);
#endif

#if ENABLE_RCSWITCH_TEST
	RcSwitch::RcSwitch_test::theTest.run();
#endif

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
