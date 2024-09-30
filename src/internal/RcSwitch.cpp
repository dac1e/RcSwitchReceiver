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

#include "RcSwitch.hpp"

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

	/** Return the pulse types that a pulse matches for this protocol */
	PulseTypes pulseToPulseTypes(const Pulse &pulse) const;
};

/** Normal level protocol group specification in microseconds: */
static const Protocol normalLevelProtocolsTable[] { // Sorted in ascending order of lowTimeRange.msecLowerBound
		//     |synch                                                    |data
		//                                                                |logical 0 data bit pulse pair                            |logical 1 data bit pulse pair
        //      |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..
        //      |..is the second pulse      |..is the first pulse         |..is the second pulse      |..is the first pulse         |..is the second pulse      |..is the first pulse
		//#p    |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound
		{    7,{{        7440,       11160},{         240,         360}},{{         720,        1080},{         120,         180}},{{         120,         180},{         720,        1080}}},
		{    1,{{        8680,       13020},{         280,         420}},{{         840,        1260},{         280,         420}},{{         280,         420},{         840,        1260}}},
		{    4,{{        1824,        2736},{         304,         456}},{{         912,        1368},{         304,         456}},{{         304,         456},{         912,        1368}}},
		{    8,{{       20800,       31200},{         480,         720}},{{        2560,        3840},{        1120,        1680}},{{        2560,        3840},{         480,         720}}},
		{    2,{{        5200,        7800},{         520,         780}},{{        1040,        1560},{         520,         780}},{{         520,         780},{        1040,        1560}}},
		{    5,{{        5600,        8400},{        2400,        3600}},{{         800,        1200},{         400,         600}},{{         400,         600},{         800,        1200}}},
		{    3,{{        5680,        8520},{        2400,        3600}},{{         880,        1320},{         320,         480}},{{         480,         720},{         720,        1080}}},
};

/** Inverse level protocol group specification in microseconds: */
static const Protocol inverseLevelProtocolsTable[] { // Sorted in ascending order of msecHighTimeLowerBound
		//     |synch                                                    |data
		//                                                                |logical 0 data bit pulse pair                            |logical 1 data bit pulse pair
        //      |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..
        //      |..is the first pulse       |..is the second pulse        |..is the first pulse       |..is the second pulse        |..is the first pulse       |..is the second pulse
		//#p    |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound
		{   13,{{         216,         324},{        7776,       11664}},{{         216,         324},{         432,         648}},{{         432,         648},{         216,         324}}},
		{   11,{{         256,         384},{        9216,       13824}},{{         256,         384},{         512,         768}},{{         512,         768},{         256,         384}}},
		{   10,{{         292,         438},{        5256,        7884}},{{         876,        1314},{         292,         438}},{{         292,         438},{         876,        1314}}},
		{    6,{{         360,         540},{        8280,       12420}},{{         360,         540},{         720,        1080}},{{         720,        1080},{         360,         540}}},
		{    9,{{        1120,        1680},{       20800,       31200}},{{        2560,        3840},{        1120,        1680}},{{        2560,        3840},{         480,         720}}},
};

/* The number of rows of the 2 above tables. */
constexpr size_t  normalLevelProtocolsTableRowCount =
		sizeof( normalLevelProtocolsTable)/sizeof( normalLevelProtocolsTable[0]);
constexpr size_t inverseLevelProtocolsTableRowCount =
		sizeof(inverseLevelProtocolsTable)/sizeof(inverseLevelProtocolsTable[0]);

bool Protocol::isNormalLevelProtocol() const {
	const bool result = (this < &normalLevelProtocolsTable[normalLevelProtocolsTableRowCount]) &&
			(this >= &normalLevelProtocolsTable[0]);
	return result;
}

bool Protocol::isInverseLevelProtocol() const {
	const bool result = this < (&inverseLevelProtocolsTable[inverseLevelProtocolsTableRowCount]) &&
			(this >= &inverseLevelProtocolsTable[0]);
	return result;
}

PulseTypes Protocol::pulseToPulseTypes(const Pulse &pulse) const {
	PulseTypes result = { PULSE_TYPE::UNKNOWN, PULSE_TYPE::UNKNOWN };
	switch (pulse.mPulseLevel) {
		case PULSE_LEVEL::LO: {
			const bool inverseLevel = isInverseLevelProtocol();
			const TimeRange::COMPARE_RESULT synchCompare =
					synchronizationPulsePair.durationLowLevelPulse.compare(pulse.mMicroSecDuration);
			if (inverseLevel) {
				/* First synch pulse might be longer, because level went from low to low */
				if (synchCompare == TimeRange::IS_WITHIN || synchCompare == TimeRange::TOO_LONG) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_FIRST_PULSE;
				}
			} else {
				RCSWITCH_ASSERT(isNormalLevelProtocol());
				if (synchCompare == TimeRange::IS_WITHIN) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_SECOND_PULSE;
				}
			}
			const TimeRange::COMPARE_RESULT log0Compare =
					logical0PulsePair.durationLowLevelPulse.compare(pulse.mMicroSecDuration);
			if (log0Compare == TimeRange::IS_WITHIN) {
				result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_0;
			} else {
				const TimeRange::COMPARE_RESULT log1Compare =
						logical1PulsePair.durationLowLevelPulse.compare(
						pulse.mMicroSecDuration);
				if (log1Compare == TimeRange::IS_WITHIN) {
					result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_1;
				}
			}
			break;
		}
		case PULSE_LEVEL::HI: {
			const bool inverseLevel = isInverseLevelProtocol();
			const TimeRange::COMPARE_RESULT synchCompare =
					synchronizationPulsePair.durationHighLevelPulse.compare(pulse.mMicroSecDuration);
			if (inverseLevel) {
				if (synchCompare == TimeRange::IS_WITHIN) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_SECOND_PULSE;
				}
			} else {
				RCSWITCH_ASSERT(isNormalLevelProtocol());
				if (synchCompare == TimeRange::IS_WITHIN) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_FIRST_PULSE;
				}
			}
			const TimeRange::COMPARE_RESULT log0Compare =
					logical0PulsePair.durationHighLevelPulse.compare(
					pulse.mMicroSecDuration);
			if (log0Compare == TimeRange::IS_WITHIN) {
				result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_0;
			} else {
				const TimeRange::COMPARE_RESULT log1Compare =
						logical1PulsePair.durationHighLevelPulse.compare(
						pulse.mMicroSecDuration);
				if (log1Compare == TimeRange::IS_WITHIN) {
					result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_1;
				}
			}
			break;
		}
		case PULSE_LEVEL::UNKNOWN:
			// Nothing to analyze
			break;
	}
	return result;
}

/** Returns the array of protocols for a protocol group. */
static inline std::pair<const Protocol*, size_t> getProtocolTable(const PROTOCOL_GROUP_ID protocolGroupId) {
	static const std::pair<const Protocol*, size_t> protocolGroups[] = {
			{ normalLevelProtocolsTable,  normalLevelProtocolsTableRowCount},
			{inverseLevelProtocolsTable, inverseLevelProtocolsTableRowCount},
	};

	return protocolGroups[protocolGroupId];
}

static inline void collectNormalLevelProtocolCandidates(
		ProtocolCandidates& protocolCandidates, const Pulse&  pulse_0, const Pulse&  pulse_1) {

	for(size_t i = 0; i < normalLevelProtocolsTableRowCount; i++) {
		const Protocol& prot = normalLevelProtocolsTable[i];
		if(pulse_0.mMicroSecDuration <
				normalLevelProtocolsTable[i].synchronizationPulsePair.durationHighLevelPulse.lowerBound) {
			/* Protocols are sorted in ascending order of synch.lowTimeRange.microSecLowerBound
			 * So further protocols will have even higher microSecLowerBound. Hence we can
			 * break here immediately.
			 */
			return;
		}

		if(pulse_0.mMicroSecDuration <
				prot.synchronizationPulsePair.durationHighLevelPulse.upperBound) {
			if(pulse_1.mMicroSecDuration >=
					prot.synchronizationPulsePair.durationLowLevelPulse.lowerBound) {
				if(pulse_1.mMicroSecDuration <
						prot.synchronizationPulsePair.durationLowLevelPulse.upperBound) {
					protocolCandidates.push(i);
				}
			}
		}
	}
}

static inline void collectInverseLevelProtocolCandidates(
		ProtocolCandidates& protocolCandidates, const Pulse&  pulse_0, const Pulse&  pulse_1) {

	for(size_t i = 0; i < inverseLevelProtocolsTableRowCount; i++) {
		const Protocol& prot = inverseLevelProtocolsTable[i];
		if(pulse_0.mMicroSecDuration <
				inverseLevelProtocolsTable[i].synchronizationPulsePair.durationLowLevelPulse.lowerBound) {
			/* Protocols are sorted in ascending order of synch.microSecHighTimeLowerBound
			 * So further protocols will have even higher microSecHighTimeLowerBound. Hence
			 * we can break here immediately. */
			return;
		}

		if(pulse_1.mMicroSecDuration >=
				prot.synchronizationPulsePair.durationHighLevelPulse.lowerBound) {
			if(pulse_1.mMicroSecDuration
					< prot.synchronizationPulsePair.durationHighLevelPulse.upperBound) {
				protocolCandidates.push(i);
			}
		}
	}
}

// ======== ProtocolCandidates =========
size_t ProtocolCandidates::getProtcolNumber(const size_t protocolCandidateIndex) const {
	 std::pair<const Protocol*, size_t> protocol = getProtocolTable(mProtocolGroupId);
	 RCSWITCH_ASSERT(protocolCandidateIndex < size());
	 const size_t protocolIndex = at(protocolCandidateIndex);
	 RCSWITCH_ASSERT(protocolIndex < protocol.second);
	 return protocol.first[protocolIndex].protocolNumber;
}

// ======== Receiver ===================
void Receiver::collectProtocolCandidates(const Pulse&  pulse_0, const Pulse&  pulse_1) {
  if(pulse_0.mPulseLevel != pulse_1.mPulseLevel) {
		if(pulse_0.mPulseLevel == PULSE_LEVEL::HI) {
			mProtocolCandidates.setProtocolGroup(NORMAL_LEVEL_PROTOCOLS);
			collectNormalLevelProtocolCandidates(mProtocolCandidates, pulse_0, pulse_1);
		} else if(pulse_0.mPulseLevel == PULSE_LEVEL::LO) {
			mProtocolCandidates.setProtocolGroup(INVERSE_LEVEL_PROTOCOLS);
			collectInverseLevelProtocolCandidates(mProtocolCandidates, pulse_0, pulse_1);
		} else {
			 /* UNKNOWN pulse level given as argument */
			RCSWITCH_ASSERT(false);
		}
  } else {
  	/* 2 subsequent pulses with same level don't make sense and will be ignored.
  	 * However, assert that no UNKNOWN* pulse level given as argument. */
	RCSWITCH_ASSERT(pulse_0.mPulseLevel != PULSE_LEVEL::UNKNOWN);
  }
}

// inline attribute, because it is private and called once.
inline PULSE_TYPE Receiver::analyzePulsePair(const Pulse& firstPulse, const Pulse& secondPulse) {
	PULSE_TYPE result = PULSE_TYPE::UNKNOWN;
	const std::pair<const Protocol*, size_t> protocols = getProtocolTable(mProtocolCandidates.getProtocolGroup());
	size_t protocolCandidatesIndex = mProtocolCandidates.size();
	while(protocolCandidatesIndex > 0) {
		--protocolCandidatesIndex;
		RCSWITCH_ASSERT(protocolCandidatesIndex < protocols.second);
		const Protocol& protocol = protocols.first[mProtocolCandidates[protocolCandidatesIndex]];

		const PulseTypes& pulseTypesSecondPulse =
				protocol.pulseToPulseTypes(secondPulse);
		const PulseTypes& pulseTypesFirstPulse =
				protocol.pulseToPulseTypes(firstPulse);

		if(pulseTypesSecondPulse.mPulseTypeSynch == PULSE_TYPE::SYNCH_SECOND_PULSE
				&& pulseTypesFirstPulse.mPulseTypeSynch == PULSE_TYPE::SYNCH_FIRST_PULSE) {
			/* The pulses match the protocol for synch pulses. */
			return PULSE_TYPE::SYCH_PULSE;
		}

		if(pulseTypesSecondPulse.mPulseTypeData == pulseTypesFirstPulse.mPulseTypeData
				&& pulseTypesSecondPulse.mPulseTypeData !=  PULSE_TYPE::UNKNOWN) {
			/* The pulses match the protocol for data pulses */
			if(result == PULSE_TYPE::UNKNOWN) { /* keep the first match */
				result = pulseTypesSecondPulse.mPulseTypeData;
			}
		} else {
			// The pulses do not match the protocol
			mProtocolCandidates.remove(protocolCandidatesIndex);
		}
	}
	return result;
}

void Receiver::handleInterrupt(const int pinLevel, const uint32_t microSecInterruptTime) {
	if(!mSuspended) {
		const uint32_t microSecDuration = microSecInterruptTime - mMicrosecLastInterruptTime;
#if DEBUG_RCSWITCH
		tracePulse(microSecDuration, pinLevel);
#endif
		push(microSecDuration, pinLevel);

		switch(state()) {
			case SYNC_STATE:
				if(size() > 1) {
					collectProtocolCandidates(at(size()-2), at(size()-1));
					/* If the above call has identified any valid protocol
					 * candidate, the state has implicitly become DATA_STATE.
					 * Refer to function state(). */
				}
				break;
			case DATA_STATE:
				if(++mDataModePulseCount == 2) {
					mDataModePulseCount = 0;
					const Pulse& firstPulse  = at(size()-2);
					const Pulse& secondPulse = at(size()-1);
					const PULSE_TYPE pulseType = analyzePulsePair(firstPulse, secondPulse);
					if(pulseType == PULSE_TYPE::UNKNOWN) {
						/* Unknown pulses received, hence start from scratch. Current pulses
						 * might be the synch start, but for a different protocol. */
						mProtocolCandidates.reset();
						/* Check current pulses for being a synch of a different protocol. */
						collectProtocolCandidates(firstPulse, secondPulse);
						retry();
					} else {
						if(pulseType == PULSE_TYPE::SYCH_PULSE) {
							/* The 2 pulses are a new sync start, we are finished
							 * with the current message package */
							if(mReceivedMessagePacket.size() >= MIN_MSG_PACKET_BITS) {
								mMessageAvailable = true;
							} else {
								/* Insufficient number of bits received, hence start from
								 * scratch. Current pulses might be the synch start, but
								 * for a different protocol. */
								mProtocolCandidates.reset();
								/* Check current pulses for being a synch of a different protocol. */
								collectProtocolCandidates(firstPulse, secondPulse);
								retry();
							}
						} else {
							/* It is a sequence of 2 data pulses */
							RCSWITCH_ASSERT(pulseType == PULSE_TYPE::DATA_LOGICAL_0
									|| pulseType == PULSE_TYPE::DATA_LOGICAL_1);
							const DATA_BIT dataBit = pulseType == PULSE_TYPE::DATA_LOGICAL_0 ?
											DATA_BIT::LOGICAL_0 : DATA_BIT::LOGICAL_1;
							mReceivedMessagePacket.push(dataBit);
						}
					}
				}
				break;
			case AVAILABLE_STATE:
				/* Do nothing. */
				break;
		}
	}
	mMicrosecLastInterruptTime = microSecInterruptTime;
}

// inline attribute, because it is private and called once.
inline void Receiver::push(uint32_t microSecDuration, const int pinLevel) {
	Pulse * const storage = beyondTop();
	*storage = {microSecDuration, (pinLevel ? PULSE_LEVEL::LO : PULSE_LEVEL::HI)
	};
	baseClass::selectNext();
}

Receiver::STATE Receiver::state() const {
	if (mMessageAvailable) {
		return AVAILABLE_STATE;
	}
	return mProtocolCandidates.size() ? DATA_STATE : SYNC_STATE;
}

// inline attribute, because it is private and called once.
inline void Receiver::retry() {
	mReceivedMessagePacket.reset();
	baseClass::reset();
}

void Receiver::reset() {
	mProtocolCandidates.reset();
	mReceivedMessagePacket.reset();
	baseClass::reset();
	/* Changing this flag must be the last action here,
	 * because it will change the state. That must not
	 * happen before all of the above reset calls are
	 * finished.
	 * Otherwise the interrupt handler may already work
	 * with dirty class members. */
	mMessageAvailable = false;
}

size_t Receiver::receivedBitsCount() const {
	if(available()) {
		const MessagePacket& messagePacket = mReceivedMessagePacket;
		return messagePacket.size() + messagePacket.overflowCount();
	}
	return 0;
}

uint32_t Receiver::receivedValue() const {
	uint32_t result = 0;
	if(available()) {
		const MessagePacket& messagePacket = mReceivedMessagePacket;
		for(size_t i=0; i< messagePacket.size(); i++) {
			result = result << 1;
			RCSWITCH_ASSERT(messagePacket.at(i) != DATA_BIT::UNKNOWN);
			if(messagePacket.at(i) == DATA_BIT::LOGICAL_1) {
				result |= 1;
			}
		}
	}
	return result;
}

int Receiver::receivedProtocol(const size_t index) const {
	if(index < mProtocolCandidates.size()) {
		return mProtocolCandidates.getProtcolNumber(index);
	}
	return -1;
}

} /* namespace RcSwitch */
