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
#include <stddef.h>
#include <stdint.h>
#include <Arduino.h>


/*
 * The remote control protocol is a stream of pulse pairs with different duration and
 * pulse levels. In the context of this documentation, the first pulse will be
 * referred to as "pulse A" and the second one as "pulse B".
 *
 *   Normal level protocols start with a high level:
 *          ___________________
 *     XXXX|                   |____________________|XXXX
 *
 *
 *
 *   Inverse level protocols start with a low level:
 *                              ____________________
 *     XXXX|___________________|                    |XXXX
 *
 *         ^                   ^                    ^
 *         | pulse A duration  | pulse B duration   |
 *
 *
 *  In the synchronization phase there is a short pulse followed by a very long pulse:
 *     Normal level protocols:
 *          ____
 *     XXXX|    |_____________________________________________________________|XXXX
 *
 *
 *
 *     Inverse level protocols:
 *               _____________________________________________________________
 *     XXXX|____|                                                             |XXXX
 *
 *
 *  In the data phase there is
 *   a short pulse followed by a long pulse for a logical 0 data bit:
 *     Normal level protocols:
 *          __
 *     XXXX|  |________|XXXX
 *
 *     Inverse level protocols:
 *             ________
 *     XXXX|__|        |XXXX
 *
 *   a long pulse followed by a short pulse for a logical 1 data bit:
 *     Normal level protocols:
 *          ________
 *     XXXX|        |__|XXXX
 *
 *     Inverse level protocols:
 *                   __
 *     XXXX|________|  |XXXX
 *
 *
 * Pulse durations sent out by a real world transmitter can vary. Hence the
 * timing specification for receiving pulses must have a time range
 * for a pulse to be recognized as a valid synchronization pulse respectively
 * data pulse.
 *
 * Synch. pulses and data pulses are defined as a multiple of a protocol
 * specific clock cycle.
 *
 * There is a decision to be made, when the received number of data bits constitute
 * a completed message packet so that further reception of data bits must be
 * stopped. It is assumed, that the transmitter transmits the same message
 * packets multiple times in a row. The completion of a message packet is
 * determined, upon receiving new synch pulses from a subsequent transmission.
 */

/**
 * makeTimingSpec
 *
 * Calculates the pulse timings specification from a given protocol specification
 * at compile time. Compile time calculation keeps the interrupt handler quick.
 * makeTimingSpec is to be used in combination with RxProtocolTable below.
 */
template<
  /** A protocol specification is given by the following parameters: */
  unsigned int protocolNumber,           /* A unique integer identifier of this protocol. */
  unsigned int usecClock,                /* The clock rate in microseconds.  */
  unsigned percentTolerance,       /* The tolerance for a pulse length to be recognized as a valid. */
  unsigned int synchA,  unsigned int synchB,   /* Number of clocks for the synchronization pulse pair. */
  unsigned int data0_A, unsigned int data0_B,  /* Number of clocks for a logical 0 bit data pulse pair. */
  unsigned int data1_A, unsigned int data1_B,  /* Number of clocks for a logical 1 bit data pulse pair. */
  bool inverseLevel>               /* Flag whether pulse levels are normal or inverse. */
struct makeTimingSpec;

/**
 * RxProtocolTable
 *
 * Provides an array of timing specifications from given protocol specifications.
 * The array gets sorted at compile time. Sort criteria are the inverseLevel
 * flag and the lowerBound of the synch A pulse.
 * Sorting the table at compile time provides an opportunity to speed up the
 * interrupt handler.
 *
 * Usage example (to be placed in your sketch, refer to example PrintReceivedData):
 *
 * static const RxProtocolTable <
 *  //                   #, clk,  %, syA,  syB,  d0A,d0B,  d1A,d1B, inverseLevel
 *    makeTimingSpec<  1, 350, 20,   1,   31,    1,  3,    3,  1, false>, // (PT2262)
 *    makeTimingSpec<  2, 650, 20,   1,   10,    1,  3,    3,  1, false>, // ()
 *    makeTimingSpec<  3, 100, 20,  30,   71,    4, 11,    9,  6, false>, // ()
 *    makeTimingSpec<  4, 380, 20,   1,    6,    1,  3,    3,  1, false>, // ()
 *    makeTimingSpec<  5, 500, 20,   6,   14,    1,  2,    2,  1, false>, // ()
 *    makeTimingSpec<  6, 450, 20,   1,   23,    1,  2,    2,  1, true>,  // (HT6P20B)
 *    makeTimingSpec<  7, 150, 20,   2,   62,    1,  6,    6,  1, false>, // (HS2303-PT)
 *    makeTimingSpec<  8, 200, 20,   3,  130,    7, 16,    3, 16, false>, // (Conrad RS-200)
 *    makeTimingSpec<  9, 365, 20,   1,   18,    3,  1,    1,  3, true>,  // (1ByOne Doorbell)
 *    makeTimingSpec< 10, 270, 20,   1,   36,    1,  2,    2,  1, true>,  // (HT12E)
 *    makeTimingSpec< 11, 320, 20,   1,   36,    1,  2,    2,  1, true>   // (SM5212)
 *  > rxProtocolTable;
 *
 *  The resulting array of timing specifications can be dumped for debug purpose:
 *  ...
 *  rxProtocolTable.dumpTimingSpec(serial);
 *  ...
 *
 */
template<typename ...TimingSpecs> struct RxProtocolTable;
#include "internal/ProtocolTimingSpec.hpp"

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
	 * Return the number of received values within one packet.
	 * When RCSWITCH_UINT32_ARRAY_SIZE greater than one, more than one value can
   * be received within a packet.
	 */
	static inline size_t receivedValuesCount() {return mReceiverDelegate.receivedValuesCount();}

  /**
   * Return the received value if a value is available. Otherwise 0.
   * The first received bit will be reflected as the highest
   * significant bit. In case that more than one value has been received within one packet
   * function receivedValueAt() can be called to obtain subsequent values.
   * See also receivedValuesCount().
   * Must not be called, when available returns false.
   */
  static inline receivedValue_t receivedValue() {return mReceiverDelegate.receivedValue();}

	/**
	 * Return the received value at a particular index, if a value at that index is available. Otherwise 0.
	 * When RCSWITCH_UINT32_ARRAY_SIZE greater than one, more than one value can
	 * be received within a packet. Subsequent values can be queried by calling this function with the next
	 * index. The highest possible index can be obtained from function receivedValuesCount().
	 * The first received bit will be reflected as the highest
	 * significant bit.
	 * Must not be called, when available returns false.
	 */
	static inline receivedValue_t receivedValueAt(const size_t index) {return mReceiverDelegate.receivedValueAt(index);}


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
