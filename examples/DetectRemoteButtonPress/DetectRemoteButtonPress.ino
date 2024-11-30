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
 */

#include "ProtocolDefinition.hpp"
#include "RcSwitchReceiver.hpp"
#include <Arduino.h>


// For details about this protocol table, refer to documentation in ProtocolDefinition.hpp
// You can add own protocols and remove not needed protocols.
// However, the number of normal level protocols as well as the number of inverse level
// Protocols should not exceed 7 in this table. Refer to MAX_PROTOCOL_CANDIDATES in RcSwitch.hpp.
static const RxProtocolTable <
//                  #, clk,  %, syA,  syB,  d0A,d0B,  d1A, d1B, inverseLevel
	makeTimingSpec<  1, 350, 20,   1,   31,    1,  3,    3,  1, false>, // ()
	makeTimingSpec<  2, 650, 20,   1,   10,    1,  3,    3,  1, false>, // ()
	makeTimingSpec<  3, 100, 20,  30,   71,    4, 11,    9,  6, false>, // ()
	makeTimingSpec<  4, 380, 20,   1,    6,    1,  3,    3,  1, false>, // ()
	makeTimingSpec<  5, 500, 20,   6,   14,    1,  2,    2,  1, false>, // ()
	makeTimingSpec<  6, 450, 20,   1,   23,    1,  2,    2,  1, true>, 	// (HT6P20B)
	makeTimingSpec<  7, 150, 20,   2,   62,    1,  6,    6,  1, false>, // (HS2303-PT)
	makeTimingSpec<  8, 200, 20,   3,  130,    7, 16,    3, 16, false>, // (Conrad RS-200)
	makeTimingSpec<  9, 365, 20,   1,   18,    3,  1,    1,  3, true>, 	// (1ByOne Doorbell)
	makeTimingSpec< 10, 270, 20,   1,   36,    1,  2,    2,  1, true>, 	// (HT12E)
	makeTimingSpec< 11, 320, 20,   1,   36,    1,  2,    2,  1, true>  	// (SM5212)
> rxProtocolTable;

constexpr int RX433_DATA_PIN = 2;
static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;

// Reference to the serial to be used for printing.
typeof(Serial)& output = Serial;


/**
 * The Remote control transmitter typically repeats sending the message
 * packet for a button as long as the button is pressed. This class
 * filters out the repeated message packages and signals a pressed remote
 * control button only once.
 */
class RcButtonPressDetector {
	typeof(rcSwitchReceiver)* mRcSwitchReceiver;

	enum class RC_BUTTON_STATE {
		OFF,
		OFF_DELAY,
		ON,
	};

	enum class RC_BUTTON {
		NONE = -1,
		A = 'A',
		B = 'B',
		C = 'C',
		D = 'D',
	};

	static constexpr unsigned OFF_DELAY_TIME = 250; // milliseconds

	RC_BUTTON_STATE mRcButtonState = RC_BUTTON_STATE::OFF;
	RC_BUTTON mLastPressedButton;
	uint32_t mOffDelayStartTime;


	const char buttonToChar(RC_BUTTON rcButton) {
		if((rcButton != RC_BUTTON::NONE)) {
			return static_cast<char>(rcButton);
		}
		return '?';
	}

	static RC_BUTTON rcDataToButton(const uint32_t receivedData) {
		RC_BUTTON button = RC_BUTTON::NONE;
		switch (receivedData) {
			case 5592332:
				button = RC_BUTTON::A;
				break;
			case 5592512:
				button = RC_BUTTON::B;
				break;
			case 5592323:
				button = RC_BUTTON::C;
				break;
			case 5592368:
				button = RC_BUTTON::D;
				break;
		}
		return button;
	}

	RC_BUTTON testRcButtonData() {
		if(mRcSwitchReceiver->available()) {
			uint32_t rcButtonValue = mRcSwitchReceiver->receivedValue();
			mRcSwitchReceiver->resetAvailable();
			return rcDataToButton(rcButtonValue);
		}
		return RC_BUTTON::NONE;
	}

	// Here the detected button is just printed.
	void signalButton(const RC_BUTTON button) {
		output.print("Detected press for button: ");
		output.println(buttonToChar(button));
	}
public:
	void begin(typeof(rcSwitchReceiver)& rcSwitchReceiver) {
		mRcSwitchReceiver = &rcSwitchReceiver;
	}

	RcButtonPressDetector()
		: mRcSwitchReceiver(nullptr), mLastPressedButton(RC_BUTTON::NONE), mOffDelayStartTime(0)
	{
	}

	void scanRcButtons() {
		const RC_BUTTON button = testRcButtonData();
		switch (mRcButtonState) {
			case RC_BUTTON_STATE::OFF: {
				if (button != RC_BUTTON::NONE) {
					// Button pressed, signal it.
					signalButton(button);
					mLastPressedButton = button;
					// Move to ON state
					mRcButtonState = RC_BUTTON_STATE::ON;
				}
				break;
			}
			case RC_BUTTON_STATE::ON: {
				if (button != RC_BUTTON::NONE) {
					// Button still pressed
					if (button != mLastPressedButton) {
						// It is a different button, signal it.
						signalButton(button);
						mLastPressedButton = button;
					}
					// Stay in ON state.
				} else {
					// Button released. Record release time and move to off delay state.
					mOffDelayStartTime = millis();
					mRcButtonState = RC_BUTTON_STATE::OFF_DELAY;
				}
				break;
			}
			case RC_BUTTON_STATE::OFF_DELAY: {
				if (button != RC_BUTTON::NONE) {
					// Button pressed again while in OFF_DELAY state.
					if (button != mLastPressedButton) {
						// It is a different button, signal it
						signalButton(button);
						mLastPressedButton = button;
					} else {
						const uint32_t time = millis();
						if ((time - mOffDelayStartTime) > OFF_DELAY_TIME) {
							// The same button was pressed again after off
							// delay has expired, signal it.
							signalButton(button);
						}
					}
					// Either a different button was pressed, or the same button
					// was pressed after off delay time expired. Move to ON state.
					mRcButtonState = RC_BUTTON_STATE::ON;
				} else {
					// All buttons still released in OFF delay state.
					const uint32_t time = millis();
					if ((time - mOffDelayStartTime) > OFF_DELAY_TIME) {
						// Off delay time expired, move to OFF state.
						mRcButtonState = RC_BUTTON_STATE::OFF;
					}
				}
				break;
			}
		}
	}
};

static RcButtonPressDetector rcButtonPressDetector;

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
	rcButtonPressDetector.begin(rcSwitchReceiver);
}

// The loop function is called in an endless loop
void loop()
{
	rcButtonPressDetector.scanRcButtons();
}
