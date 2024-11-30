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

#include "ProtocolDefinition.hpp"
#include "internal/RcSwitch.hpp"
#include <Arduino.h>

using RcSwitch::rxTimingSpecTable;

/**
 * This is the library API class for receiving data from a remote control.
 * The IO pin to be used is defined at compile time by the template
 * parameter IOPIN. If template parameter PULSE_TRACES_COUNT is set to a
 * value greater than 0, the last received pulses can be dumped. This
 * is helpful for analyzing the protocol of a remote control transmitter.
 * There is an example sketch TraceReceivedPulses.ino shipped along with
 * this library to demonstrate how pulses can be traced.
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
	static void begin(const rxTimingSpecTable& rxTimingSpecTable) {
		pinMode(IOPIN, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(IOPIN), handleInterrupt, CHANGE);
		mReceiverDelegate.setRxTimingSpecTable(rxTimingSpecTable);
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
	static inline uint32_t receivedValue() {return mReceiverDelegate.receivedValue();}

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
	static inline 	size_t receivedProtocolCount() {return mReceiverDelegate.receivedProtocolCount();}

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
	 */
	static inline int receivedProtocol(const size_t index = 0)
		{return mReceiverDelegate.receivedProtocol(index);}

	/**
	 * Clear the last received value in order to receive a new one.
	 * Can be called at any time.
	 */
	static inline void resetAvailable() {if(mReceiverDelegate.available()) {mReceiverDelegate.reset();}}

	/**
	 * Suspend receiving new message packets.
	 */
	static void suspend() {mReceiverDelegate.suspend();}

	/**
	 * Resume receiving new message packets.
	 */
	static void resume() {mReceiverDelegate.resume();}

	/**
	 * Dump the most recent received pulses, starting with the youngest pulse.
	 * Only available if PULSE_TRACES_COUNT is greater than zero. Otherwise
	 * you'll see a compiler error here.
	 */
	static void dumpPulseTracer(typeof(Serial)& serial) {
		mReceiverDelegate.dumpPulseTracer(serial);
	}

	/**
	 * Return a reference to the internal receiver that this API class forwards
	 * it's public function calls to.
	 */
	static basicReceiver_t getReceiverDelegate() {
		return mReceiverDelegate;
	}
};

/** The receiver instance for this IO pin. */
template<int IOPIN, size_t PULSE_TRACES_COUNT> typename RcSwitchReceiver<IOPIN, PULSE_TRACES_COUNT>::receiver_t
	RcSwitchReceiver<IOPIN, PULSE_TRACES_COUNT>::mReceiverDelegate;

#endif /* RCSWITCH_RECEIVER_API_HPP_ */
