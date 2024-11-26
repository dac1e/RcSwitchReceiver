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

#include "RcSwitchReceiver.hpp"
#include <Arduino.h>

// Trace the last 64 received pulses from the remote control.
constexpr size_t TRACE_BUFFER_SIZE = 128;
constexpr int RX433_DATA_PIN = 2;

// Passing a trace buffer size greater 0 will enable RcSwitchReceiver tracing capability.
static RcSwitchReceiver<RX433_DATA_PIN, TRACE_BUFFER_SIZE> rcSwitchReceiver;

// Reference to the serial to be used for printing.
typeof(Serial)& output = Serial;

// Connect a push button to pin  13 and ground.
constexpr int TRIGGER_BUTTON = 13;
int lastbuttonState = LOW;

// The setup function is called once at startup of the sketch.
void setup()
{
	output.begin(9600);

	// Not interested in receiving data, suspend scanning for data.
	rcSwitchReceiver.suspend();

	// No timing spec. table required when not interested in receiving data.
	rcSwitchReceiver.begin(rxTimingSpecTable{nullptr, 0});

	pinMode(TRIGGER_BUTTON, INPUT_PULLUP);
	int lastbuttonState = digitalRead(TRIGGER_BUTTON);
}

// The loop function is called in an endless loop.
void loop()
{
	// Dump the most recently received pulses upon pressing the button on TRIGGER_BUTTON.
	// The remote control typically repeats its message package as long as a remote
	// control button is pressed. So you can keep the remote control button pressed,
	// and pushing the TRIGGER_BUTTON to watch the received pulses and then release
	// the remote control button.

	// If you just press the TRIGGER_BUTTON without any transmission from a remote control,
	// you will see random pulses that come from HF noise.
	const int buttonState = digitalRead(TRIGGER_BUTTON);
	if(lastbuttonState == HIGH && buttonState == LOW) {
		output.println("===========================");
		//
		// Dump the most recent received pulses, starting with the youngest pulse.
		//
		rcSwitchReceiver.dumpPulseTracer(output);
	}
	lastbuttonState = buttonState;
}
