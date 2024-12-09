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

#include "RcButtonPressDetector.hpp"
#include <Arduino.h>

/**
 * The Remote control transmitter typically repeats sending the message
 * packet for a button as long as the button is pressed. This class
 * filters out the repeated message packages and signals a pressed remote
 * control button only once.
 */
RcButtonPressDetector::rcButtonCode_t RcButtonPressDetector::testRcButtonData() {
	rcButtonCode_t result = NO_BUTTON;
	if(mRcSwitchReceiver->available()) {
		const uint32_t rcButtonValue = mRcSwitchReceiver->receivedValue();
		int i = 0;
		while(true) { // loop over matching protocols until we get a button code for the received value;
			const int rcProtocol = mRcSwitchReceiver->receivedProtocol(i);
			if(rcProtocol < 0) {
				break;
			}
			const rcButtonCode_t rcButtonCode = rcDataToButton(rcProtocol, rcButtonValue);
			if(rcButtonCode != RcButtonPressDetector::NO_BUTTON) {
				result = rcButtonCode;
				break;
			}
			++i;
		}
	}
	mRcSwitchReceiver->resetAvailable();
	return result;
}

RcButtonPressDetector::rcButtonCode_t RcButtonPressDetector::rcDataToButton(
		const int rcProtocol, const receivedValue_t receivedData) const {
	return RcButtonPressDetector::NO_BUTTON;
}

RcButtonPressDetector::RcButtonPressDetector(unsigned int msecDebounceDelayTime)
	: mDebounceDelayTime(msecDebounceDelayTime)
	, mRcSwitchReceiver(nullptr)
	, mLastPressedButton(NO_BUTTON)
	, mOffDelayStartTime(0)
{
}

void RcButtonPressDetector::scanRcButtons() {
	const rcButtonCode_t button = testRcButtonData();
	switch (mRcButtonState) {
		case STATE::OFF: {
			if (button != NO_BUTTON) {
				// Button pressed, signal it.
				onButtonPressed(button);
				mLastPressedButton = button;
				// Move to ON state
				mRcButtonState = STATE::ON;
			}
			break;
		}
		case STATE::ON: {
			if (button != NO_BUTTON) {
				// Button still pressed
				if (button != mLastPressedButton) {
					// It is a different button, signal it.
					onButtonPressed(button);
					mLastPressedButton = button;
				}
				// Stay in ON state.
			} else {
				// Button released. Record release time and move to off delay state.
				mOffDelayStartTime = millis();
				mRcButtonState = STATE::OFF_DELAY;
			}
			break;
		}
		case STATE::OFF_DELAY: {
			if (button != NO_BUTTON) {
				// Button pressed again while in OFF_DELAY state.
				if (button != mLastPressedButton) {
					// It is a different button, signal it
					onButtonPressed(button);
					mLastPressedButton = button;
				} else {
					const uint32_t time = millis();
					if ((time - mOffDelayStartTime) > mDebounceDelayTime) {
						// The same button was pressed again after off
						// delay has expired, signal it.
						onButtonPressed(button);
					}
				}
				// Either a different button was pressed, or the same button
				// was pressed after off delay time expired. Move to ON state.
				mRcButtonState = STATE::ON;
			} else {
				// All buttons still released in OFF delay state.
				const uint32_t time = millis();
				if ((time - mOffDelayStartTime) > mDebounceDelayTime) {
					// Off delay time expired, move to OFF state.
					mRcButtonState = STATE::OFF;
				}
			}
			break;
		}
	}
}
