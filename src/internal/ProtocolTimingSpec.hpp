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

#ifndef RCSWITCH_PROTOCOL_TIMING_SPEC_HPP_
#define RCSWITCH_PROTOCOL_TIMING_SPEC_HPP_

#include <sys/types.h>
#include <stdint.h>

#include "typeselect.hpp"

namespace RcSwitch {

/*
 * The protocol is a stream of pulse pairs with different duration and pulse levels.
 * In the context of this documentation, the first pulse will be referenced as
 * "pulse A" and the second one as "pulse B".
 *
 *   Normal level protocols start with a high level:
 *          ___________________
 *	   XXXX|                   |____________________|XXXX
 *
 *   Inverse level protocols start with a low level:
 *                              ____________________
 *	   XXXX|___________________|                    |XXXX
 *
 *	       ^                   ^                    ^
 *         | pulse A duration  | pulse B duration   |
 *
 *
 *  In the synchronization phase there is a short pulse followed by a very long pulse:
 *     Normal level protocols:
 *          ____
 *     XXXX|    |_____________________________________________________________|XXXX
 *
 *     Inverse level protocols:
 *               _____________________________________________________________
 *     XXXX|____|                                                             |XXXX
 *
 *
 *  In the data phase there is
 *   a short pulse followed by a long pulse for a logical 0 data bit:
 *     Normal level protocols:
 *           __
 *     XXXXX|  |________|XXXX
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
 * specification contains upper and lower boundaries for a pulse to be
 * recognized as a valid synchronization pulse respectively data pulse.
 *
 * Synch. pulses and data pulses are typically a multiple of a protocol
 * specific clock cycle.
 *
 * There is a decision to be made, when a the reception of data bits shall be stopped,
 * because they constitute a completed message packet. Here is assumed, that the
 * transmitter transmits the same message packets multiple times in a row. The
 * completion of a message packet is noted, when new synch pulses from a subsequent
 * transmission appear.
 */

struct TimeRange {
	size_t lowerBound;
	size_t upperBound;

	enum COMPARE_RESULT {
		IS_WITHIN =  0,
		TOO_SHORT = -1,
		TOO_LONG  = -1,
	};

	COMPARE_RESULT compare(uint32_t value) const;
};

inline TimeRange::COMPARE_RESULT TimeRange::compare(uint32_t value) const {
	if(value <  lowerBound) {return TOO_SHORT;}
	if(value >= upperBound) {return TOO_LONG;}
	return IS_WITHIN;
}

struct RxPulsePairTimeRanges {
	TimeRange durationA;
	TimeRange durationB;
};

struct RxTimingSpec {
	size_t   protocolNumber;
	bool bInverseLevel;
	RxPulsePairTimeRanges  synchronizationPulsePair; // synch
	RxPulsePairTimeRanges  data0pulsePair;
	RxPulsePairTimeRanges  data1pulsePair;

	/** Return true, if this protocol is an inverse level protocol.
	 * Otherwise false. */
	bool isInverseLevelProtocol() const {
		return bInverseLevel;
	}

	/** Return true, if this protocol is a normal level protocol.
	 * Otherwise false. */
	bool isNormalLevelProtocol() const {
		return not bInverseLevel;
	}
};

struct TxPulsePairTiming {
	size_t durationA;
	size_t durationB;
};

struct TxTimingSpec { /* Currently only required for test */
	size_t   protocolNumber;
	bool bInverseLevel;
	TxPulsePairTiming	 synchPulsePair;    // pulse pair for synch
	TxPulsePairTiming	 data0PulsePair;	// pulse pair for logical 0
	TxPulsePairTiming	 data1PulsePair;    // pulse pair for logical 1
};

template<typename L, typename R> struct isRxTimingSpecLower {
	static constexpr bool value = L::INVERSE_LEVEL == R::INVERSE_LEVEL ?
			(L::usecSynchA_lowerBound < R::usecSynchA_lowerBound) : L::INVERSE_LEVEL < R::INVERSE_LEVEL;
};

} // namespace RcSwitch

/**
 * makeTimingSpec
 * Calculate the pulse timings specification from a given protocol specification at compile time.
 * To be used i combination with RxProtocolTable.
 */
template<
	/** A protocol specification is given by the following parameters: */
	size_t protocolNumber,					/* A unique integer identifier for this protocol. */
	size_t clock,							/* The clock rate in microseconds.  */
	size_t percentTolerance,				/* The tolerance for a pulse length to be recognized as a valid. */
	size_t synchA,  size_t synchB,			/* Number of clocks for the synchronization pulse pair. */
	size_t data0_A, size_t data0_B,			/* Number of clocks for a logical 0 bit data pulse pair. */
	size_t data1_A, size_t data1_B,			/* Number of clocks for a logical 1 bit data pulse pair. */
	bool inverseLevel = false>				/* Flag whether pulse levels are normal or inverse. */

struct makeTimingSpec { // Calculate the timing specification from the protocol definition.
	static constexpr size_t PROTOCOL_NUMBER = protocolNumber;
	static constexpr bool INVERSE_LEVEL = inverseLevel;

	static constexpr size_t uSecSynchA = clock * synchA;
	static constexpr size_t uSecSynchB = clock * synchB;
	static constexpr size_t usecSynchA_lowerBound = uSecSynchA * (100-percentTolerance) / 100;
	static constexpr size_t usecSynchA_upperBound = uSecSynchA * (100+percentTolerance) / 100;
	static constexpr size_t usecSynchB_lowerBound = uSecSynchB * (100-percentTolerance) / 100;
	static constexpr size_t usecSynchB_upperBound = uSecSynchB * (100+percentTolerance) / 100;

	static constexpr size_t uSecData0_A = clock * data0_A;
	static constexpr size_t uSecData0_B = clock * data0_B;
	static constexpr size_t uSecData0_A_lowerBound = uSecData0_A * (100-percentTolerance) / 100;
	static constexpr size_t uSecData0_A_upperBound = uSecData0_A * (100+percentTolerance) / 100;
	static constexpr size_t uSecData0_B_lowerBound = uSecData0_B * (100-percentTolerance) / 100;
	static constexpr size_t uSecData0_B_upperBound = uSecData0_B * (100+percentTolerance) / 100;

	static constexpr size_t uSecData1_A = clock * data1_A;
	static constexpr size_t uSecData1_B = clock * data1_B;
	static constexpr size_t uSecData1_A_lowerBound = uSecData1_A * (100-percentTolerance) / 100;
	static constexpr size_t uSecData1_A_upperBound = uSecData1_A * (100+percentTolerance) / 100;
	static constexpr size_t uSecData1_B_lowerBound = uSecData1_B * (100-percentTolerance) / 100;
	static constexpr size_t uSecData1_B_upperBound = uSecData1_B * (100+percentTolerance) / 100;

	typedef RcSwitch::RxTimingSpec rx_spec_t;
	static constexpr rx_spec_t RX = {PROTOCOL_NUMBER, INVERSE_LEVEL,
		{	/* synch pulses */
			{usecSynchA_lowerBound, usecSynchA_upperBound},     {usecSynchB_lowerBound, usecSynchB_upperBound}
		},
		{   /* LOGICAL_0 data bit logical pulses */
			{uSecData0_A_lowerBound, uSecData0_A_upperBound}, {uSecData0_B_lowerBound, uSecData0_B_upperBound}
		},
		{
			/* LOGICAL_1 data bit logical pulses */
			{uSecData1_A_lowerBound, uSecData1_A_upperBound}, {uSecData1_B_lowerBound, uSecData1_B_upperBound}
		},
	};

	template<typename T> struct IS_RX_LOWER {
		static constexpr bool value = inverseLevel == T::inverseLevel ?
				(usecSynchA_lowerBound < T::usecSynchA_lowerBound) : inverseLevel < T::inverseLevel;
	};

	/* Currently only required for tests */
	typedef RcSwitch::TxTimingSpec tx_spec_t;
	static constexpr tx_spec_t TX = {protocolNumber, inverseLevel,
		{	/* synch pulses */
			uSecSynchA, uSecSynchB
		},
		{   /* LOGICAL_0 data bit logical pulses */
			uSecData0_A, uSecData0_B
		},
		{
			/* LOGICAL_1 data bit logical pulses */
			uSecData1_A, uSecData1_B
		},
	};
};

/**
 * RxProtocolTable provides an array of timing specifications from given protocol specifications.
 * The array gets sorted at compile time to keep the interrupt handler quick. Sort criteria are
 * the inverseLevel flag and the lowerBound of the synch A pulse.
 *
 * Usage example:
 *
 * static const RxProtocolTable <
 *  //                   #, clk,  %, syA,  syB,  d0A,d0B,  d1A,d1B , inverseLevel
 *  	makeTimingSpec<  1, 350, 20,   1,   31,    1,  3,    3,  1>, 		// ()
 *  	makeTimingSpec<  2, 650, 20,   1,   10,    1,  3,    3,  1>, 		// ()
 *  	makeTimingSpec<  3, 100, 20,  30,   71,    4, 11,    9,  6>, 		// ()
 *  	makeTimingSpec<  4, 380, 20,   1,    6,    1,  3,    3,  1>, 		// ()
 *  	makeTimingSpec<  5, 500, 20,   6,   14,    1,  2,    2,  1>, 		// ()
 *  	makeTimingSpec<  6, 450, 20,   1,   23,    1,  2,    2,  1, true>, 	// (HT6P20B)
 *  	makeTimingSpec<  7, 150, 20,   2,   62,    1,  6,    6,  1>, 		// (HS2303-PT)
 *  	makeTimingSpec<  8, 200, 20,   3,  130,    7, 16,    3, 16>, 		// (Conrad RS-200)
 *  	makeTimingSpec<  9, 365, 20,   1,   18,    3,  1,    1,  3, true>, 	// (1ByOne Doorbell)
 *  	makeTimingSpec< 10, 270, 20,   1,   36,    1,  2,    2,  1, true>, 	// (HT12E)
 *  	makeTimingSpec< 11, 320, 20,   1,   36,    1,  2,    2,  1, true>  	// (SM5212)
 *  > rxProtocolTable;
 */
template<typename ...Ts> struct
RxProtocolTable {
private:
	using T = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::selected;
	using R = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::rest;
	const RcSwitch::RxTimingSpec* toArray() const {return &m;}
public:
	static constexpr size_t ROW_COUNT =	sizeof(RxProtocolTable) / sizeof(RcSwitch::RxTimingSpec);
	RcSwitch::RxTimingSpec m = T::RX;
	RxProtocolTable<R> r;

	typedef std::pair<const RcSwitch::RxTimingSpec*, size_t> rxTimingSpecTable_t;
	// Convert to rxTimingSpecTable_t
	inline rxTimingSpecTable_t toTimingSpecTable() const {
		constexpr size_t rowCount = ROW_COUNT;
		return std::make_pair(toArray(), rowCount);
	}
};

/**
 * RxProtocolTable specialization for a table with just 1 row.
 */
template<typename T> struct
RxProtocolTable<T> {
private:
	const RcSwitch::RxTimingSpec* toArray() const {return &m;}
public:
	static constexpr size_t ROW_COUNT =	sizeof(RxProtocolTable) / sizeof(RcSwitch::RxTimingSpec);
	RcSwitch::RxTimingSpec m = T::RX;

	typedef std::pair<const RcSwitch::RxTimingSpec*, size_t> rxTimingSpecTable_t;
	// Convert to rxTimingSpecTable_t
	inline rxTimingSpecTable_t toTimingSpecTable() const {
		constexpr size_t rowCount = ROW_COUNT;
		return std::make_pair(toArray(), rowCount);
	}
};

/**
 * For internal use only.
 * RxProtocolTable specialization for timing specs being passed as a tuple.
 */
template<typename ...Ts> struct
RxProtocolTable<std::tuple<Ts...>> {
private:
	using T = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::selected;
	using R = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::rest;
public:
	RcSwitch::RxTimingSpec m = T::RX;
	RxProtocolTable<R> r;
};

/**
 * For internal use only.
 * RxProtocolTable specialization for a single timing spec being passed as a tuple.
 */
template<typename T> struct
RxProtocolTable<std::tuple<T>> {
	RcSwitch::RxTimingSpec m = T::RX;
};



#endif /* RCSWITCH_PROTOCOL_TIMING_SPEC_HPP_ */
