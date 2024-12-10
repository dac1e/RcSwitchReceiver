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

/**
 * This sketch helps you to analyze the pulses of your remote control as a debugging feature.
 */

// The remote control typically repeats its message package as long as a remote
// control button is pressed.
// The following procedure helps you to record the pulses of a remote control:
// 0) Wire a push button (TRIGGER_BUTTON) between pin13 an GND of your Arduino.
// 1) Place the remote control closed to the receiver, so that you record clean pulses.
// 2) Press any remote control button and keep it pressed.
// 3) After 3 seconds, push the TRIGGER_BUTTON and watch the received pulses on the serial monitor.
// 4) Release the remote control button even while pulses are still dumped on the serial monitor.


// Note: If you just press the TRIGGER_BUTTON without any transmission from a remote control,
// you will see random pulses that come from HF noise.

#include "ProtocolDefinition.hpp"
#include "RcSwitchReceiver.hpp"
#include <Arduino.h>

// Trace the last 140 received pulses from the remote control.
constexpr size_t TRACE_BUFFER_SIZE = 140;

#if defined (ARDUINO_AVR_UNO)
constexpr int RX433_DATA_PIN = 2; // Pin 2 has interrupt capability on UNO
#else
constexpr int RX433_DATA_PIN = 6;
#endif

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

	// Not interested in decoding RC data, suspend scanning for data.
	rcSwitchReceiver.suspend();

	// No timing spec. table required when not interested in decoding RC data.
	rcSwitchReceiver.begin(RxTimingSpecTable{nullptr, 0});

	pinMode(TRIGGER_BUTTON, INPUT_PULLUP);
	int lastbuttonState = digitalRead(TRIGGER_BUTTON);
}

// The loop function is called in an endless loop.
void loop()
{
	const int buttonState = digitalRead(TRIGGER_BUTTON);
	if(lastbuttonState == HIGH && buttonState == LOW) {
		//
		// Dump from the oldest to the newest pulses.
		//
//		rcSwitchReceiver.dumpPulseTracer(output, ","); Use this for excel csv format where comma is separator.
//		rcSwitchReceiver.dumpPulseTracer(output, ";"); Use this for excel csv format where semicolon is separator.
		rcSwitchReceiver.dumpPulseTracer(output, "");
	}
	lastbuttonState = buttonState;
}
