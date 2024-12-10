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
#include "RcButtonPressDetector.hpp"
#include <Arduino.h>

unsigned int constexpr PROTOCOL_PT2262  = 1;

// For details about this protocol table, refer to documentation in ProtocolDefinition.hpp
// You can add own protocols and remove not needed protocols.
// You may use the expample sketch LearnRemoteControl.ino to find out what you need to enter here.
// However, the number of normal level protocols as well as the number of inverse level
// Protocols should not exceed 7 in this table.
DATA_ISR_ATTR static const RxProtocolTable <
	//                              #, clk,  %, syA,  syB,  d0A,d0B,  d1A,d1B, inverseLevel
	makeTimingSpec< PROTOCOL_PT2262  , 350, 20,   1,   31,    1,  3,    3,  1, false> // (PT2262)
> rxProtocolTable;

#if defined (ARDUINO_AVR_UNO)
constexpr int RX433_DATA_PIN = 2; // Pin 2 has interrupt capability on UNO
#else
constexpr int RX433_DATA_PIN = 6;
#endif

static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;

// Reference to the serial to be used for printing.
typeof(Serial)& output = Serial;

class MyRcButtonPressDetector : public RcButtonPressDetector { // @suppress("Class has a virtual method and non-virtual destructor")
	const char buttonToChar(rcButtonCode_t buttonCode) const {
		if((buttonCode != NO_BUTTON)) {
			return static_cast<char>(buttonCode);
		}
		return '?';
	}

	rcButtonCode_t rcDataToButton(const int rcProtocol, receivedValue_t receivedData) const override {
		rcButtonCode_t buttonCode = RcButtonPressDetector::rcDataToButton(rcProtocol, receivedData);
		if(rcProtocol == PROTOCOL_PT2262) {
			switch (receivedData) {
			case 5592332:
				buttonCode = 'A';
				break;
			case 5592512:
				buttonCode = 'B';
				break;
			case 5592323:
				buttonCode = 'C';
				break;
			case 5592368:
				buttonCode = 'D';
				break;
			}
		}
		return buttonCode;
	}

	// The detected button is printed here and the builtin led is set.
	void onButtonPressed(rcButtonCode_t buttonCode) const override {
		output.print("You pressed button ");
		output.println(buttonToChar(buttonCode));
		if(buttonCode == 'A' || buttonCode == 'C') {
			// Switch on upon button A and C
			digitalWrite(LED_BUILTIN, HIGH);
		} else if(buttonCode == 'B' || buttonCode == 'D') {
			// Switch off upon button B and D
			digitalWrite(LED_BUILTIN, LOW);
		}
	}
};

static MyRcButtonPressDetector rcButtonPressDetector;

// The setup function is called once at startup of the sketch
void setup()
{
	output.begin(9600);
	pinMode(LED_BUILTIN, OUTPUT);
	rcSwitchReceiver.begin(rxProtocolTable.toTimingSpecTable());
	rcButtonPressDetector.begin(rcSwitchReceiver);
}

// The loop function is called in an endless loop
void loop()
{
	rcButtonPressDetector.scanRcButtons();
}
