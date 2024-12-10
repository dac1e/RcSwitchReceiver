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

/**
 * Please read "hints on remote operating distance" in README.md
 * For wiring diagram refer to https://github.com/dac1e/RcSwitchReceiver/blob/main/extras/RcSwitchReceiverWiring.pdf
 */

#include "ProtocolDefinition.hpp"
#include "RcSwitchReceiver.hpp"
#include <Arduino.h>

// For curiosity or debugging print the timing spec table that has been created.
#define DUMP_TIMING_SPEC_TABLE true

// For details about this protocol table, refer to documentation in ProtocolDefinition.hpp
// You can add own protocols and remove not needed protocols.
// You may use the expample sketch LearnRemoteControl.ino to find out what you need to enter here.
// However, the number of normal level protocols as well as the number of inverse level
// Protocols should not exceed 7 in this table.
DATA_ISR_ATTR static const RxProtocolTable <
	//               #, clk,  %, syA,  syB,  d0A,d0B,  d1A,d1B, inverseLevel
	makeTimingSpec<  1, 350, 20,   1,   31,    1,  3,    3,  1, false>, // (PT2262)
	makeTimingSpec<  2, 650, 20,   1,   10,    1,  3,    3,  1, false>, // ()
	makeTimingSpec<  3, 100, 20,  30,   71,    4, 11,    9,  6, false>, // ()
	makeTimingSpec<  4, 380, 20,   1,    6,    1,  3,    3,  1, false>, // ()
	makeTimingSpec<  5, 500, 20,   6,   14,    1,  2,    2,  1, false>, // ()
	makeTimingSpec<  6, 450, 20,   1,   23,    1,  2,    2,  1, true>, 	// (HT6P20B)
	makeTimingSpec<  7, 150, 20,   2,   62,    1,  6,    6,  1, false>, // (HS2303-PT)
	makeTimingSpec<  8, 200, 20,   3,  130,    7, 16,    3, 16, false>, // (Conrad RS-200)
	makeTimingSpec<  9, 365, 20,   1,   18,    3,  1,    1,  3, true>, 	// (1ByOne Doorbell)
	makeTimingSpec< 10, 270, 20,   1,   36,    1,  2,    2,  1, true>, 	// (HT12E)
	makeTimingSpec< 11, 320, 20,   1,   36,    1,  2,    2,  1, true>, 	// (SM5212)
	// Note that last row must not end with a comma.
	makeTimingSpec< 12, 300, 20,   2,   23,    2,  4,    4,  2, false>  // (Sygonix)
> rxProtocolTable;

#if defined (ARDUINO_AVR_UNO)
constexpr int RX433_DATA_PIN = 2; // Pin 2 has interrupt capability on UNO
#else
constexpr int RX433_DATA_PIN = 6;
#endif

static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;

// Reference to the serial to be used for printing.
typeof(Serial)& output = Serial;

// The setup function is called once at startup of the sketch
void setup()
{
	output.begin(9600);
#if DUMP_TIMING_SPEC_TABLE
	output.println();
	rxProtocolTable.dumpTimingSpec(output);
	output.println();

	// Allow time to finalize printing the table.
	delay(500);
#endif
	rcSwitchReceiver.begin(rxProtocolTable.toTimingSpecTable());
}

// The loop function is called in an endless loop
void loop()
{
	if (rcSwitchReceiver.available()) {
		const uint32_t value = rcSwitchReceiver.receivedValue();
		output.print("Received ");
		output.print(value);


		const size_t n = rcSwitchReceiver.receivedProtocolCount();
		output.print(" / Protocol number");
		if(n > 1) {
			output.print("s:");
		} else {
			output.print(':');
		}

		for(size_t i = 0; i < n; i++) {
			const int protocolNumber = rcSwitchReceiver.receivedProtocol(i);
			output.print(' ');
			output.print(protocolNumber);
		}

		output.println();

		rcSwitchReceiver.resetAvailable();
	}
}
