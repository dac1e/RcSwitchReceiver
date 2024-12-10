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

#define PRINT_DETECTED_BUTTON true

unsigned int constexpr PROTOCOL_PT2262  = 1;
unsigned int constexpr PROTOCOL_SYGONIX =12;

// For details about this protocol table, refer to documentation in ProtocolDefinition.hpp
// You can add own protocols and remove not needed protocols.
// You may use the expample sketch LearnRemoteControl.ino to find out what you need to enter here.
// However, the number of normal level protocols as well as the number of inverse level
// Protocols should not exceed 7 in this table.
DATA_ISR_ATTR static const RxProtocolTable <
	//                              #, clk,  %, syA,  syB,  d0A,d0B, d1A, d1B, inverseLevel
	makeTimingSpec<  PROTOCOL_PT2262 , 350, 20,   1,   31,    1,  3,    3,  1, false>, // (PT2262)
    // Note that last row must not end with a comma.
	makeTimingSpec< PROTOCOL_SYGONIX , 300, 20,   2,   23,    2,  4,    4,  2, false>  // (Sygonix)
> rxProtocolTable;

#if defined (ARDUINO_AVR_UNO)
constexpr int RX433_DATA_PIN = 2; // Pin 2 has interrupt capability on UNO
#else
constexpr int RX433_DATA_PIN = 6;
#endif

static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;

// Reference to the serial to be used for printing.
typeof(Serial)& output = Serial;

enum class BUTTON_CODE : int { // Note: valid button codes must not be negative.
	// PT2262
	A = 0,
	B,
	C,
	D,

	// Sygonix ON
	I_1_ON,
	I_2_ON,
	I_3_ON,
	I_ALL_ON,

	// Sygonix OFF
	I_1_OFF,
	I_2_OFF,
	I_3_OFF,
	I_ALL_OFF,
};

#if PRINT_DETECTED_BUTTON
static const char* const BUTTON_TEXT[] = { // Must be ordered as in enum BUTTON_CODE
		"A","B","C","D",
		"I_1_ON","I_2_ON","I_3_ON","I_ALL_ON","I_1_OFF","I_2_OFF","I_3_OFF","I_ALL_OFF",
};
#endif

class MyRcButtonPressDetector : public RcButtonPressDetector { // @suppress("Class has a virtual method and non-virtual destructor")
#if PRINT_DETECTED_BUTTON
	static const char* buttonToText(rcButtonCode_t buttonCode) {
		if((buttonCode >= 0 && buttonCode < (sizeof(BUTTON_TEXT) / sizeof(BUTTON_TEXT[0])) )) {
			return BUTTON_TEXT[buttonCode];
		}
		return "?";
	}
#endif

	static bool isLedOnButton(rcButtonCode_t buttonCode) {
		bool result = false;
		switch(static_cast<BUTTON_CODE>(buttonCode)) {
		case BUTTON_CODE::A:
		case BUTTON_CODE::C:
		case BUTTON_CODE::I_1_ON:
		case BUTTON_CODE::I_2_ON:
		case BUTTON_CODE::I_3_ON:
		case BUTTON_CODE::I_ALL_ON:
			result = true;
			break;
		default:
			break;
		}
		return result;
	}

	static bool isLedOffButton(rcButtonCode_t buttonCode) {
		bool result = false;
		switch(static_cast<BUTTON_CODE>(buttonCode)) {
		case BUTTON_CODE::B:
		case BUTTON_CODE::D:
		case BUTTON_CODE::I_1_OFF:
		case BUTTON_CODE::I_2_OFF:
		case BUTTON_CODE::I_3_OFF:
		case BUTTON_CODE::I_ALL_OFF:
			result = true;
			break;
		default:
			break;
		}
		return result;
	}

	rcButtonCode_t rcDataToButton(const int rcProtocol, receivedValue_t receivedData) const override {
		BUTTON_CODE buttonCode = static_cast<BUTTON_CODE>(RcButtonPressDetector::rcDataToButton(rcProtocol, receivedData));
		if(rcProtocol == PROTOCOL_PT2262) {
			switch (receivedData) {
			case 5592332:
				buttonCode = BUTTON_CODE::A;
				break;
			case 5592512:
				buttonCode = BUTTON_CODE::B;
				break;
			case 5592323:
				buttonCode = BUTTON_CODE::C;
				break;
			case 5592368:
				buttonCode = BUTTON_CODE::D;
				break;
			}
		} else if(rcProtocol == PROTOCOL_SYGONIX) {
			switch (receivedData) {
			case 2389209600:
				buttonCode = BUTTON_CODE::I_1_ON;
				break;
			case 2791862784:
				buttonCode = BUTTON_CODE::I_2_ON;
				break;
			case 2523427328:
				buttonCode = BUTTON_CODE::I_3_ON;
				break;
			case 2473095680:
				buttonCode = BUTTON_CODE::I_ALL_ON;
				break;
			case 2171105792:
				buttonCode = BUTTON_CODE::I_1_OFF;
				break;
			case 2926080512:
				buttonCode = BUTTON_CODE::I_2_OFF;
				break;
			case 2657645056:
				buttonCode = BUTTON_CODE::I_3_OFF;
				break;
			case 2741531136:
				buttonCode = BUTTON_CODE::I_ALL_OFF;
				break;
			}
		}
		return static_cast<rcButtonCode_t>(buttonCode);
	}

	// The detected button is printed here and the builtin led is set.
	void onButtonPressed(rcButtonCode_t buttonCode) const override {
#if PRINT_DETECTED_BUTTON
		output.print("You pressed button ");
		output.println(buttonToText(buttonCode));
#endif
		if(isLedOnButton(buttonCode)) {
			// Switch on upon button A and C
			digitalWrite(LED_BUILTIN, HIGH);
		} else if(isLedOffButton(buttonCode)) {
			// Switch off upon button B and D
			digitalWrite(LED_BUILTIN, LOW);
		}
	}

public:
	MyRcButtonPressDetector()
		: RcButtonPressDetector(400) // Use 400ms instead of 250ms default for debounce
	{
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
