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
	TimeRange durationLowLevelPulse;
	TimeRange durationHighLevelPulse;
};

struct Protocol {
	size_t   protocolNumber;
	PulsePairTiming  synchronizationPulsePair;
	PulsePairTiming  logical0PulsePair;
	PulsePairTiming  logical1PulsePair;

	/** Return true, if this protocol is an inverse level protocol.
	 * Otherwise false. */
	bool isInverseLevelProtocol() const;

	/** Return true, if this protocol is a normal level protocol.
	 * Otherwise false. */
	bool isNormalLevelProtocol() const;

};

std::pair<const Protocol*, size_t> getProtocolTable(const size_t protocolGroupId);

} /* namespace RcSwitc */

#endif /* RCSWITCH_INTERNAL_PROTOCOL_HPP_ */
