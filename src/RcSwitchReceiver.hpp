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

#ifndef RCSWITCHRECEIVER_HPP_
#define RCSWITCHRECEIVER_HPP_

#include "internal/RcSwitch.hpp"
#include "internal/ProtocolTimingSpec.hpp"
#include "Arduino.h"

using RcSwitch::rxTimingSpecTable_t;

/**
 * This is the library API class for receiving data from a remote control.
 * The IO pin to be used is defined at compile time by the template
 * parameter IOPIN.
 *
 * Multiple RcSwitchReceiver can be instantiated for different IO pins.
 * E.g. if you have a 433Mhz receiver hardware connected to pin 5 and a
 * 315Mhz receiver hardware to pin 6 you can create 2 RcSwitchReceiver
 * instances as follows:
 *
 * RcSwitchReceiver<5> rcSwitchReceiver433;
 * RcSwitchReceiver<6> rcSwitchReceiver315;
 */

template<int IOPIN> class RcSwitchReceiver {
	static RcSwitch::Receiver mReceiver;

	static void handleInterrupt() {
		mReceiver.handleInterrupt(digitalRead(IOPIN), micros());
	}
public:

	/**
	 * Sets the protocol timing specification table to be used for receiving data.
	 * Sets up the receiver to receive interrupts from the IOPIN.
	 */
	static void begin(const rxTimingSpecTable_t& rxTimingSpecTable) {
		pinMode(IOPIN, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(IOPIN), handleInterrupt, CHANGE);
		mReceiver.setRxProtocolTable(rxTimingSpecTable);
	}

	/**
	 * Returns true, when a new received value is available.
	 * Can be called at any time.
	 */
	static inline bool available() {return mReceiver.available();}

	/**
	 * Return the received value if a value is available. Otherwise 0.
	 * The first received bit will be reflected as the highest
	 * significant bit.
	 * Must not be called, when available returns false.
	 */
	static inline uint32_t receivedValue() {return mReceiver.receivedValue();}

	/**
	 * Return the number of received bits. Can be greater than
	 * MAX_MSG_PACKET_BITS. Trailing bits that couldn't be
	 * stored will be cut off. The constant MAX_MSG_PACKET_BITS
	 * can be increased to avoid such an overflow.
	 * Must not be called, when available returns false.
	 */
	static inline size_t receivedBitsCount() {return mReceiver.receivedBitsCount();}

	/**
	 * Return the number of protocols that matched the synch and
	 * data pulses for the received value.
	 * Must not be called, when available returns false.
	 */
	static inline 	size_t receivedProtocolCount() {return mReceiver.receivedProtocolCount();}

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
		{return mReceiver.receivedProtocol(index);}

	/**
	 * Clear the last received value in order to receive a new one.
	 * Can be called at any time.
	 */
	static inline void resetAvailable() {if(mReceiver.available()) {mReceiver.reset();}}

	/**
	 * Suspend receiving new message packets.
	 */
	static void suspend() {mReceiver.suspend();}

	/**
	 * Resume receiving new message packets.
	 */
	static void resume() {mReceiver.resume();}

	/**
	 * Print out the receiver's Rx timing table. Main purpose is for debugging.
	 */
	static void dumpRxTimingTable(UARTClass& serial) {mReceiver.dumpRxTimingTable(serial);}

	/**
	 * Print out any Rx timing table. Main purpose is for debugging.
	 */
	static void dumpRxTimingTable(UARTClass& serial, const rxTimingSpecTable_t& rxTimingSpecTable)
	{
		mReceiver.dumpRxTimingTable(serial, rxTimingSpecTable);
	}
};

/** The receiver instance for this IO pin. */
template<int IOPIN> RcSwitch::Receiver RcSwitchReceiver<IOPIN>::mReceiver;

#endif /* RCSWITCHRECEIVER_HPP_ */
