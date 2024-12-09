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

#pragma once


#ifndef RCSWITCH_RCBUTTONPRESSDETECTOR_API_HPP_
#define RCSWITCH_RCBUTTONPRESSDETECTOR_API_HPP_

#include "internal/RcSwitch.hpp"
/**
 * The Remote control transmitter typically repeats sending the message
 * packet for a button as long as the button is pressed. This class
 * filters out the repeated message packages and signals a signal pressed
 * remote control button only once.
 */
class RcButtonPressDetector {
public:
	using receivedValue_t = RcSwitch::receivedValue_t;
	using rcButtonCode_t = int;
	static constexpr rcButtonCode_t NO_BUTTON = -1;

private:
	enum class STATE {
		OFF,
		OFF_DELAY,
		ON,
	};

	const unsigned int mDebounceDelayTime; // in milliseconds
	RcSwitch::Receiver* mRcSwitchReceiver;
	STATE mRcButtonState = STATE::OFF;
	rcButtonCode_t mLastPressedButton;
	uint32_t mOffDelayStartTime;

	rcButtonCode_t testRcButtonData();

	/**
	 * This pure virtual function will be called, when a button
	 * press has been detected.
	 * It must be overridden by the derived class.
	 *
	 *  Example of an override function:
	 *
	 *  // MyRcButtonPressDetector header file:
	 *  ...
	 *  MyRcButtonPressDetector : public RcButtonPressDetector {
	 *   	...
	 *  	void signalButton(rcButtonCode_t buttonCode) const override;
	 *  	...
	 *  };
	 *
	 *  // MyRcButtonPressDetector cpp file:
	 *  ...
	 *	void MyRcButtonPressDetector::signalButton(rcButtonCode_t buttonCode) const {
	 *		Serial.print("Button pressed: ")
	 *		Serial.println(buttonCode)
	 *	}
	 *
	 */
	virtual void onButtonPressed(rcButtonCode_t buttonCode) const = 0;

protected:
	/**
	 * This virtual function must be overridden, and provide positive button
	 * codes for all received data packet that should be evaluated.
	 * For received packets that are not of interest, the derived function
	 * must call this base class implementation.
	 *
	 *  Example of an override function:
	 *
	 *  // MyRcButtonPressDetector header file:
	 *  ...
	 *  MyRcButtonPressDetector : public RcButtonPressDetector {
	 *   	...
	 *  	rcButtonCode_t rcDataToButton(const int rcProtocol,
	 *  		uint32_t receivedData) const override;
	 *  	...
	 *  };
	 *
	 *  // MyRcButtonPressDetector cpp file:
	 *  ...
	 *	RcButtonPressDetector::rcButtonCode_t MyRcButtonPressDetector::rcDataToButton(
	 *		const int rcProtocol, receivedValue_t receivedData) const {
	 *		rcButtonCode_t buttonCode = -1;
	 *		switch (receivedData) {
	 *			case 5592332:
	 * 				buttonCode = 'A';
	 *				break;
	 *			case 5592512:
	 *				buttonCode = 'B';
	 *				break;
	 *			case 5592323:
	 *				buttonCode = 'C';
	 *				break;
	 *			case 5592368:
	 *				buttonCode = 'D';
	 *				break;
	 *			default:
	 *				buttonCode =
	 *					RcButtonPressDetector::rcDataToButton(rcProtocol, receivedData);
	 *				break;
	 *		}
	 *		return buttonCode;
	 *	}
	 *  ...
	 *
	 */
	virtual rcButtonCode_t rcDataToButton(const int rcProtocol, const receivedValue_t receivedData) const;

public:
	RcButtonPressDetector(unsigned int msecDebounceDelayTime = 250);
	void scanRcButtons();

	/**
	 * Attach the RcSwitchReceiver to this button detector.
	 *
	 * Example:
	 *	static const RxProtocolTable <
	 *	//                   #, clk,  %, syA,  syB,  d0A,d0B,  d1A, d1B, inverseLevel
	 *		makeTimingSpec<  1, 350, 20,   1,   31,    1,  3,    3,  1, false>
	 *	> rxProtocolTable;
	 *
	 *	constexpr int RX433_DATA_PIN = 2;
	 *	static RcSwitchReceiver<RX433_DATA_PIN> rcSwitchReceiver;
	 *
	 *	MyRcButtonPressDetector : public RcButtonPressDetector {
	 *		...
	 *	};
	 *
	 *  static MyRcButtonPressDetector rcButtonDetector;
	 *
	 *  void setup()
	 *  {
	 * 		...
	 * 		rcSwitchReceiver.begin(rxProtocolTable.toTimingSpecTable());
	 * 		rcButtonDetector.begin(rcSwitchReceiver);
	 *  }
	 *
	 * 	void loop() {
	 * 		...
	 * 	}
	 */
	template<int IOPIN, size_t PULSE_TRACES_COUNT>
	void begin(RcSwitchReceiver<IOPIN, PULSE_TRACES_COUNT>& rcSwitchReceiver) {
		mRcSwitchReceiver = &rcSwitchReceiver.getReceiverDelegate();
	}
};




#endif /* RCSWITCH_RCBUTTONPRESSDETECTOR_API_HPP_ */
