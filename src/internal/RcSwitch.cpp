/*
 * RcSwtichReceiver.cpp
 *
 *  Created on: 11.09.2024
 *      Author: Wolfgang
 */

#include "RcSwitch.hpp"

namespace RcSwitch {

static bool isInverseLevelProtocol(const size_t protocolGroupId);

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

	/** Return the pulse types that a pulse matches for this protocol */
	PulseTypes pulseToPulseTypes(const Pulse &pulse, const size_t protocolGroupId) const;
};

/** Timings of the normal level protocols in microseconds: first synch pulse is high. */
static const Protocol normalLevelProtocols[] { // Sorted in ascending order of lowTimeRange.msecLowerBound
		//     |synch                                                    |data
		//#p    |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound
		{    7,{{        7440,       11160},{         240,         360}},{{         720,        1080},{         120,         180}},{{         120,         180},{         720,        1080}}},
		{    1,{{        8680,       13020},{         280,         420}},{{         840,        1260},{         280,         420}},{{         280,         420},{         840,        1260}}},
		{    4,{{        1824,        2736},{         304,         456}},{{         912,        1368},{         304,         456}},{{         304,         456},{         912,        1368}}},
		{    8,{{       20800,       31200},{         480,         720}},{{        2560,        3840},{        1120,        1680}},{{        2560,        3840},{         480,         720}}},
		{    2,{{        5200,        7800},{         520,         780}},{{        1040,        1560},{         520,         780}},{{         520,         780},{        1040,        1560}}},
		{    5,{{        5600,        8400},{        2400,        3600}},{{         800,        1200},{         400,         600}},{{         400,         600},{         800,        1200}}},
		{    3,{{        5680,        8520},{        2400,        3600}},{{         880,        1320},{         320,         480}},{{         480,         720},{         720,        1080}}},
};

/** Timings of the normal level protocols in microseconds: first synch pulse is low. */
static const Protocol inverseLevelProtocols[] { // Sorted in ascending order of msecHighTimeLowerBound
		//     |synch                                                    |data
		//#p    |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound
		{   13,{{         216,         324},{        7776,       11664}},{{         216,         324},{         432,         648}},{{         432,         648},{         216,         324}}},
		{   11,{{         256,         384},{        9216,       13824}},{{         256,         384},{         512,         768}},{{         512,         768},{         256,         384}}},
		{   10,{{         292,         438},{        5256,        7884}},{{         876,        1314},{         292,         438}},{{         292,         438},{         876,        1314}}},
		{    6,{{         360,         540},{        8280,       12420}},{{         360,         540},{         720,        1080}},{{         720,        1080},{         360,         540}}},
		{    9,{{        1120,        1680},{       20800,       31200}},{{        2560,        3840},{        1120,        1680}},{{        2560,        3840},{         480,         720}}},
};

PulseTypes Protocol::pulseToPulseTypes(const Pulse &pulse, const size_t protocolGroupId) const {
	PulseTypes result = { PULSE_TYPE::UNKNOWN, PULSE_TYPE::UNKNOWN };
	switch (pulse.mPulseLevel) {
		case PULSE_LEVEL::LO: {
			const bool inverseLevel = isInverseLevelProtocol(protocolGroupId); // inversität sollte man aus dem protocol erkennen.
			const TimeRange::COMPARE_RESULT synchCompare =
					synchronizationPulsePair.durationLowLevelPulse.compare(pulse.mMicroSecDuration);
			if (inverseLevel) {
				/* First synch pulse might be longer, because level went from low to low */
				if (synchCompare == TimeRange::IS_WITHIN || synchCompare == TimeRange::TOO_LONG) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_FIRST_PULSE;
				}
			} else {
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
			const bool inverseLevel = isInverseLevelProtocol(protocolGroupId);
			const TimeRange::COMPARE_RESULT synchCompare =
					synchronizationPulsePair.durationHighLevelPulse.compare(pulse.mMicroSecDuration);
			if (inverseLevel) {
				if (synchCompare == TimeRange::IS_WITHIN) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_SECOND_PULSE;
				}
			} else {
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

constexpr size_t inverseLevelProtocolsSize = sizeof(inverseLevelProtocols)/sizeof(inverseLevelProtocols[0]);
constexpr size_t normalLevelProtocolsSize  = sizeof( normalLevelProtocols)/sizeof( normalLevelProtocols[0]);

const std::pair<const Protocol*, size_t> protocolGroups[] = {
		{ normalLevelProtocols,  normalLevelProtocolsSize},
		{inverseLevelProtocols, inverseLevelProtocolsSize},
};

static inline std::pair<const Protocol*, size_t> getProtocols(const PROTOCOL_GROUP_ID protocolGroupId) {
	return protocolGroups[protocolGroupId];
}

static inline bool isInverseLevelProtocol(const size_t protocolGroupId) {
	RCSWITCH_ASSERT(protocolGroupId < sizeof(protocolGroups) / sizeof(protocolGroups[0]));
	RCSWITCH_ASSERT(protocolGroups[0].first == normalLevelProtocols);
	RCSWITCH_ASSERT(protocolGroups[1].first == inverseLevelProtocols);
	return protocolGroupId > 0;
}

static inline void collectNormalLevelProtocolCandidates(
		ProtocolCandidates& protocolCandidates, const Pulse&  pulse_0, const Pulse&  pulse_1) {

	for(size_t i = 0; i < normalLevelProtocolsSize; i++) {
		const Protocol& prot = normalLevelProtocols[i];
		if(pulse_0.mMicroSecDuration <
				normalLevelProtocols[i].synchronizationPulsePair.durationHighLevelPulse.lowerBound) {
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

	for(size_t i = 0; i < inverseLevelProtocolsSize; i++) {
		const Protocol& prot = inverseLevelProtocols[i];
		if(pulse_0.mMicroSecDuration <
				inverseLevelProtocols[i].synchronizationPulsePair.durationLowLevelPulse.lowerBound) {
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
	 std::pair<const Protocol*, size_t> protocol = getProtocols(mProtocolGroupId);
	 RCSWITCH_ASSERT(protocolCandidateIndex < size());
	 const size_t protocolIndex = at(protocolCandidateIndex);
	 RCSWITCH_ASSERT(protocolIndex < protocol.second);
	 return protocol.first[protocolIndex].protocolNumber;
}

// ======== ReceivedMessage ============
void ReceivedMessage::reset() {
	baseClass::reset();
	for (element_type &messagePacket : mData) {
		messagePacket.reset();
	}
}

bool ReceivedMessage::isEqual(const size_t index_a, const size_t index_b) {
	RCSWITCH_ASSERT(index_a < mSize && index_b < mSize);
	if (index_a != index_b) {
		return at(index_a) == at(index_b);
	}
	return true;
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
  	/* Assert that no UNKNOWN pulse level given as argument */
		RCSWITCH_ASSERT(pulse_0.mPulseLevel != PULSE_LEVEL::UNKNOWN);
  }
}

// inline attribute, because it is private and called once.
inline PULSE_TYPE Receiver::analyzePulsePair(const Pulse& firstPulse, const Pulse& secondPulse) {
	PULSE_TYPE result = PULSE_TYPE::UNKNOWN;
	const std::pair<const Protocol*, size_t> protocols = getProtocols(mProtocolCandidates.getProtocolGroup());
	size_t protocolCandidatesIndex = mProtocolCandidates.size();
	while(protocolCandidatesIndex > 0) {
		--protocolCandidatesIndex;
		RCSWITCH_ASSERT(protocolCandidatesIndex < protocols.second);
		const Protocol& protocol = protocols.first[mProtocolCandidates[protocolCandidatesIndex]];

		const PulseTypes& pulseTypesSecondPulse =
				protocol.pulseToPulseTypes(secondPulse, mProtocolCandidates.getProtocolGroup());
		const PulseTypes& pulseTypesFirstPulse =
				protocol.pulseToPulseTypes(firstPulse, mProtocolCandidates.getProtocolGroup());

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
				 * candidate, state has implicitly become DATA_STATE.
				 * Refer to function state(). */
			}
			break;
		case DATA_STATE:
			if(++mReceivedDataModePulseCount == 2) {
				mReceivedDataModePulseCount = 0;
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
						const MessagePacket* const messagePacket = mReceivedMessage.beyondTop();
						if(messagePacket->size() >=  MIN_MSG_PACKET_BITS) {
							mReceivedMessage.selectNext();
							if(mReceivedMessage.size() >= mReceivedMessage.capacity) {
								// Enough message packets received.
								if(mReceivedMessage.capacity > 1) {
									if(!mReceivedMessage.isEqual(mReceivedMessage.capacity-2
											, mReceivedMessage.capacity-1)) {
										/* Received messages differs, hence start from scratch. */
										/* Current pulses might be the synch start, but
										 * for a different protocol.
										 */
										mProtocolCandidates.reset();
										/* Check current pulses for being a synch of a different protocol. */
										collectProtocolCandidates(firstPulse, secondPulse);
										retry();
									}
								}
							} /* else we have received sufficient consistent
								 * message packages, so state has implicitly
								 * become AVAILABLE_STATE. Refer to function state(). */
#if false
							else {
								/* Just to set be able to set a breakpoint for the debugger here. */
								const STATE currentState = state();
							}
#endif
						} else {
							/* New synch start without sufficient less data bits received. */
							mProtocolCandidates.reset();
							collectProtocolCandidates(firstPulse, secondPulse);
							retry();
						}

					} else {
						/* It is a sequence of 2 data pulses */
						RCSWITCH_ASSERT(pulseType == PULSE_TYPE::DATA_LOGICAL_0
								|| pulseType == PULSE_TYPE::DATA_LOGICAL_1);
						const DATA_BIT dataBit = pulseType == PULSE_TYPE::DATA_LOGICAL_0 ?
										DATA_BIT::LOGICAL_0 : DATA_BIT::LOGICAL_1;
						mReceivedMessage.beyondTop()->push(dataBit);
					}
				}
			}
			break;
		case AVAILABLE_STATE:
			/* Do nothing. */
			break;
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
	if (!mReceivedMessage.canGrow()) {
		return AVAILABLE_STATE;
	}
	return mProtocolCandidates.size() ? DATA_STATE : SYNC_STATE;
}

// inline attribute, because it is private and called once.
inline void Receiver::retry() {
	baseClass::reset();
	/* This reset must be the last action, because it will change the mode */
	mReceivedMessage.reset();
}

void Receiver::reset() {
	mProtocolCandidates.reset();
	retry();
}

bool Receiver::receivedBitsCount() const {
	if(available()) {
		const MessagePacket& messagePacket = mReceivedMessage[0];
		return messagePacket.size() + messagePacket.overflowCount();
	}
	return 0;
}

uint32_t Receiver::receivedValue() const {
	uint32_t result = 0;
	if(available()) {
		const MessagePacket& messagePacket = mReceivedMessage[0];
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
