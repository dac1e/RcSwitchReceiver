/*
  RcSwitchReceiver - Arduino libary for remote control receiver Copyright (c)
  2024 Wolfgang Schmieder.  All right reserved.

  Contributors:
  - Wolfgang Schmieder

  Project home: https://github.com/dac1e/RcSwitchReceiver/

  This library is free software; you can redistribute it and/or modify it
  the terms of the GNU Lesser General Public License as under published
  by the Free Software Foundation; either version 3.0 of the License,
  or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/


//#include "test/RcSwitch_test.hpp" // Comment in, if you want to run the test.
#include "RcSwitchReceiver.hpp"
#include <Arduino.h>

// Change to true, if you want to print the timing spec table that has been created
#define DUMP_TIMING_SPEC_TABLE true

// Add own protocols and remove not needed protocols.
static const RxProtocolTable <
//                   #, clk,  %, syA,  syB,  d0A,d0B,  d1A,d1B , inverseLevel
	makeTimingSpec<  1, 350, 20,   1,   31,    1,  3,    3,  1>, 		// ()
	makeTimingSpec<  2, 650, 20,   1,   10,    1,  3,    3,  1>, 		// ()
	makeTimingSpec<  3, 100, 20,  30,   71,    4, 11,    9,  6>, 		// ()
	makeTimingSpec<  4, 380, 20,   1,    6,    1,  3,    3,  1>, 		// ()
	makeTimingSpec<  5, 500, 20,   6,   14,    1,  2,    2,  1>, 		// ()
	makeTimingSpec<  6, 450, 20,   1,   23,    1,  2,    2,  1, true>, 	// (HT6P20B)
	makeTimingSpec<  7, 150, 20,   2,   62,    1,  6,    6,  1>, 		// (HS2303-PT)
	makeTimingSpec<  8, 200, 20,   3,  130,    7, 16,    3, 16>, 		// (Conrad RS-200)
	makeTimingSpec<  9, 365, 20,   1,   18,    3,  1,    1,  3, true>, 	// (1ByOne Doorbell)
	makeTimingSpec< 10, 270, 20,   1,   36,    1,  2,    2,  1, true>, 	// (HT12E)
	makeTimingSpec< 11, 320, 20,   1,   36,    1,  2,    2,  1, true>  	// (SM5212)
> rxProtocolTable;

constexpr int RX433_DATA_PIN = 6;
static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;

// Reference to the serial to be used for printing.
UARTClass& serial = Serial3;

// The setup function is called once at startup of the sketch
void setup()
{
#if ENABLE_RCSWITCH_TEST
	RcSwitch::RcSwitch_test::theTest.run();
#endif

	serial.begin(9600);
	delay(100);

#if DUMP_TIMING_SPEC_TABLE
	serial.println();
	rxProtocolTable.dumpTimingSpec(serial);
	serial.println();
#endif

	rcSwitchReceiver.begin(rxProtocolTable.toTimingSpecTable());
	delay(500);
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
