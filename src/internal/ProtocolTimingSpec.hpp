/*
 * Protocol.hpp
 *
 *  Created on: 01.10.2024
 *      Author: Wolfgang
 */

#pragma once

#ifndef RCSWITCH_PROTOCOL_TIMING_SPEC_HPP_
#define RCSWITCH_PROTOCOL_TIMING_SPEC_HPP_

#include <sys/types.h>
#include <stdint.h>

#include "typeselect.hpp"

#define DEBUG_RCSWITCH_PROTOCOL_SPEC true

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
 * The pulse duration specification for the different protocols are stored in 2 arrays
 *   normalLevelProtocolsTable[]
 *  inverseLevelProtocolsTable[]
 *
 * Pulse durations sent out by a real world transmitter can vary. Hence the
 * specification contains upper and lower boundaries for a pulse to be
 * recognized as a valid synch. or data pulse.
 *
 * Synch. pulses and data pulses are typically a multiple of a protocol specific clock
 * cycle. The specification tables contain already pre-calculated
 * durations to keep the interrupt handler quick. The lower / upper boundary tolerance
 * is +- 20%.
 *
 * The protocol specs. are also sorted by a particular column within the table to
 * speed up pulse validation. That helps to keep the interrupt handler quick.
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
	TxPulsePairTiming	 synchPulsePair;
	TxPulsePairTiming	 data0PulsePair;
	TxPulsePairTiming	 data1PulsePair;
};

template<typename L, typename R> struct isRxTimingSpecLower {
	static constexpr bool value = L::INVERSE_LEVEL == R::INVERSE_LEVEL ?
			(L::usecSynchA_lowerBound < R::usecSynchA_lowerBound) : L::INVERSE_LEVEL < R::INVERSE_LEVEL;
};

} // namespace RcSwitch


template<size_t protocolNumber, size_t percentTolerance, size_t clock, size_t synchA, size_t synchB, size_t data0_A, size_t data0_B, size_t data1_A, size_t data1_B, bool inverseLevel = false>
struct makeProtocolTimingSpec {
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


// RxProtocolTable, sorted automatically at compile time by inverseLevel flag and usecSynchA_lowerBound
template<typename ...Ts> struct
RxProtocolTable {
private:
	using T = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::selected;
	using R = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::rest;
	static const RcSwitch::RxTimingSpec* toArray(const RxProtocolTable& rxSpecTable) {
		return reinterpret_cast<const RcSwitch::RxTimingSpec*>(&rxSpecTable.m);
	}
public:
	static constexpr size_t ROW_COUNT = sizeof(RxProtocolTable) / sizeof(RcSwitch::RxTimingSpec);
	RcSwitch::RxTimingSpec m = T::RX;
	RxProtocolTable<R> r;
	const RcSwitch::RxTimingSpec* toArray() const {return toArray(*this);}
};

template<typename T> struct
RxProtocolTable<T> {
private:
	static const RcSwitch::RxTimingSpec* toArray(const RxProtocolTable& rxSpecTable) {
		return reinterpret_cast<const RcSwitch::RxTimingSpec*>(&rxSpecTable.m);
	}
public:
	static constexpr size_t ROW_COUNT = sizeof(RxProtocolTable) / sizeof(RcSwitch::RxTimingSpec);
	RcSwitch::RxTimingSpec m = T::RX;
	const RcSwitch::RxTimingSpec* toArray() const {return toArray(*this);}
};

template<typename ...Ts> struct
RxProtocolTable<std::tuple<Ts...>> {
private:
	using T = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::selected;
	using R = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::rest;
public:
	RcSwitch::RxTimingSpec m = T::RX;
	RxProtocolTable<R> r;
};

template<typename T> struct
RxProtocolTable<std::tuple<T>> {
	RcSwitch::RxTimingSpec m = T::RX;
};



#endif /* RCSWITCH_PROTOCOL_TIMING_SPEC_HPP_ */
