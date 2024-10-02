/*
 * Protocol.hpp
 *
 *  Created on: 01.10.2024
 *      Author: Wolfgang
 */

#pragma once

#ifndef RCSWITCH_INTERNAL_PROTOCOL_HPP_
#define RCSWITCH_INTERNAL_PROTOCOL_HPP_

#include <stdint.h>
#include <sys/types.h>
#include <tuple>

#define DEBUG_RCSWITCH_PROTOCOL_DEF true

namespace RcSwitch {

/*
 * The protocol is a stream of pulse pairs with different duration and pulse levels.
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
 *         |1st pulse duration | 2nd pulse duration |
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
	uint32_t lowerBound;
	uint32_t upperBound;

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

struct PulsePairTiming {
	TimeRange durationA; // A
	TimeRange durationB; // B
};

struct Protocol {
	size_t   protocolNumber;
	PulsePairTiming  synchronizationPulsePair; // synch
	PulsePairTiming  logical0PulsePair;	// data0
	PulsePairTiming  logical1PulsePair; // data1

	/** Return true, if this protocol is an inverse level protocol.
	 * Otherwise false. */
	bool isInverseLevelProtocol() const;

	/** Return true, if this protocol is a normal level protocol.
	 * Otherwise false. */
	bool isNormalLevelProtocol() const;

};

std::pair<const Protocol*, size_t> getProtocolTable(const size_t protocolGroupId);

template<size_t protocolNumber, size_t percentTolerance, size_t clock, size_t synchA, size_t synchB, size_t data0_A, size_t data0_B, size_t data1_A, size_t data1_B>
struct MakeProtocol {
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

	static constexpr Protocol VALUE = {protocolNumber,
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
};


} /* namespace RcSwitc */

#if DEBUG_RCSWITCH_PROTOCOL_DEF
	#include <UartClass.h>
	namespace RcSwitch {
		void printProtocolTable(UARTClass& serial, const size_t protocolGroup);
	}
#endif

#endif /* RCSWITCH_INTERNAL_PROTOCOL_HPP_ */
