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

#ifndef RCSWITCH_RECEIVER_API_HPP_
#define RCSWITCH_RECEIVER_API_HPP_

#include "internal/ISR_ATTR.hpp"
#include "internal/RcSwitch.hpp"
#include "ProtocolDefinition.hpp"
#include <Arduino.h>

using RcSwitch::RxTimingSpecTable;

/**
 * This is the library API class for receiving data from a remote control.
 * The IO pin to be used is defined at compile time by the template
 * parameter IOPIN. If template parameter PULSE_TRACES_COUNT is set to a
 * value greater than 0, the last received pulses can be dumped and
 * analyzed. This helpful for determining the pulse timing of a remote
 * control transmitter.
 * There is an example sketch TraceReceivedPulses.ino shipped along with
 * this library to demonstrate how pulses can be analyzed.
 *
 * Multiple RcSwitchReceiver can be instantiated for different IO pins.
 * E.g. if you have a 433Mhz receiver hardware connected to pin 5 and a
 * 315Mhz receiver hardware to pin 6 you can create 2 RcSwitchReceiver
 * instances as follows:
 *
 * RcSwitchReceiver<5> rcSwitchReceiver433;
 * RcSwitchReceiver<6> rcSwitchReceiver315;
 */

template<int IOPIN, size_t PULSE_TRACES_COUNT = 0> class RcSwitchReceiver {
public:
	using receiver_t = typename RcSwitch::ReceiverSelector<PULSE_TRACES_COUNT>::receiver_t;
	using receivedValue_t = RcSwitch::receivedValue_t;
	using basicReceiver_t = RcSwitch::Receiver;
private:
	static receiver_t mReceiverDelegate;

	TEXT_ISR_ATTR_0 static void handleInterrupt() {
		const unsigned long time = micros();
		const int pinLevel = digitalRead(IOPIN);
		mReceiverDelegate.handleInterrupt(pinLevel, time);
	}
public:
	/**
	 * Sets the protocol timing specification table to be used for receiving data.
	 * Sets up the receiver to receive interrupts from the IOPIN.
	 */
	static void begin(const RxTimingSpecTable& rxTimingSpecTable) {
		pinMode(IOPIN, INPUT_PULLUP);
		mReceiverDelegate.setRxTimingSpecTable(rxTimingSpecTable);
		attachInterrupt(digitalPinToInterrupt(IOPIN), handleInterrupt, CHANGE);
	}

	/**
	 * Returns true, when a new received value is available.
	 * Can be called at any time.
	 */
	static inline bool available() {return mReceiverDelegate.available();}

	/**
	 * Return the received value if a value is available. Otherwise 0.
	 * The first received bit will be reflected as the highest
	 * significant bit.
	 * Must not be called, when available returns false.
	 */
	static inline receivedValue_t receivedValue() {return mReceiverDelegate.receivedValue();}

	/**
	 * Return the number of received bits. Can be greater than
	 * MAX_MSG_PACKET_BITS. Trailing bits that couldn't be
	 * stored will be cut off. The constant MAX_MSG_PACKET_BITS
	 * can be increased to avoid such an overflow.
	 * Must not be called, when available returns false.
	 */
	static inline size_t receivedBitsCount() {return mReceiverDelegate.receivedBitsCount();}

	/**
	 * Return the number of protocols that matched the synch and
	 * data pulses for the received value.
	 * Must not be called, when available returns false.
	 */
	static inline size_t receivedProtocolCount() {return mReceiverDelegate.receivedProtocolCount();}

	/**
	 * Return the protocol number that matched the synch and data
	 * pulses for the received value. The index can be
	 * enumerated. -1 is returned if the index is invalid.
	 * Must not be called, when available returns false.
	 *
	 * Example:
	 *
	 * if(available) {
	 *	 const size_t n = rcSwitchReceiver.receivedProtocolCount();
	 *	 Serial.print(" / Protocol number");
	 *	 if(n > 1) {
	 *		 Serial.print("s:");
	 *	 } else {
	 *		 Serial.print(':');
	 *	 }
	 *
	 *	 for(size_t i = 0; i < n; i++) {
	 *	 const int protocolNumber = rcSwitchReceiver.receivedProtocol(i);
	 *	 Serial.print(' ');
	 *		 Serial.print(protocolNumber);
	 *	 }
	 *
	 *   Serial.println();
	 * }
	 *
	 * Warning: Call resetAvailable() will clear the receivedProtocols
	 * of the received value:
	 */
	static inline int receivedProtocol(const size_t index = 0)
		{return mReceiverDelegate.receivedProtocol(index);}

	/**
	 * Clear the last received value in order to receive a new one.
	 * Will also clear the received protocols that the last
	 * received value belongs to. Can be called at any time.
	 */
	static inline void resetAvailable() {mReceiverDelegate.resetAvailable();}

	/**
	 * Suspend receiving new message packets.
	 */
	static void suspend() {mReceiverDelegate.suspend();}

	/**
	 * Resume receiving new message packets.
	 */
	static void resume() {mReceiverDelegate.resume();}

	/**
	 * Dump the oldest to the youngest pulse as well as pulse statistics.
	 */
	static void dumpPulseTracer(typeof(Serial)& serial, const char* separator = "") {
		RcSwitch::ReceiverSelector<PULSE_TRACES_COUNT>::dumpPulseTracer(mReceiverDelegate, serial, separator);
	}

	/**
	 * Deduce protocol and dump the result on the serial monitor.
	 */
	static void deduceProtocolFromPulseTracer(typeof(Serial)& serial) {
		RcSwitch::ReceiverSelector<PULSE_TRACES_COUNT>::deduceProtocolFromPulseTracer(mReceiverDelegate, serial);
	}

	/**
	 * Return a reference to the internal receiver that this API class forwards
	 * it's public function calls to.
	 */
	static basicReceiver_t& getReceiverDelegate() {
		return mReceiverDelegate;
	}
private:
	static constexpr bool IS_SMALL_PROCESSOR = sizeof(size_t) <= 2;
	static constexpr size_t PULSE_TRACES_LIMIT = IS_SMALL_PROCESSOR ? 140 : 280;

	static_assert((PULSE_TRACES_COUNT <= PULSE_TRACES_LIMIT),
			"Error: Maximum number for parameter PULSE_TRACES_COUNT exceeded. "
			"The need for static RAM scales with the number of traced pulses "
			"and the likelihood of a stack overflow scales with the consumption "
			"of static RAM. This is critical for micro controllers with very "
			"little RAM like on Arduino UNO R3 with ATmega328P.");
};

/** The receiver instance for this IO pin. */
template<int IOPIN, size_t PULSE_TRACES_COUNT> typename RcSwitchReceiver<IOPIN, PULSE_TRACES_COUNT>::receiver_t
	RcSwitchReceiver<IOPIN, PULSE_TRACES_COUNT>::mReceiverDelegate;

#endif /* RCSWITCH_RECEIVER_API_HPP_ */
