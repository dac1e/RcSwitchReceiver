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
#include "Protocol.hpp"

namespace RcSwitch {


static PulseTypes pulseToPulseTypes(const Protocol& protocol, const Pulse &pulse) {
	PulseTypes result = { PULSE_TYPE::UNKNOWN, PULSE_TYPE::UNKNOWN };
	switch (pulse.mPulseLevel) {
		case PULSE_LEVEL::LO: {
			const bool inverseLevel = protocol.isInverseLevelProtocol();
			const TimeRange::COMPARE_RESULT synchCompare =
					protocol.synchronizationPulsePair.durationLowLevelPulse.compare(pulse.mMicroSecDuration);
			if (inverseLevel) {
				/* First synch pulse might be longer, because level went from low to low */
				if (synchCompare == TimeRange::IS_WITHIN || synchCompare == TimeRange::TOO_LONG) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_FIRST_PULSE;
				}
			} else {
				RCSWITCH_ASSERT(protocol.isNormalLevelProtocol());
				if (synchCompare == TimeRange::IS_WITHIN) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_SECOND_PULSE;
				}
			}
			const TimeRange::COMPARE_RESULT log0Compare =
					protocol.logical0PulsePair.durationLowLevelPulse.compare(pulse.mMicroSecDuration);
			if (log0Compare == TimeRange::IS_WITHIN) {
				result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_0;
			} else {
				const TimeRange::COMPARE_RESULT log1Compare =
						protocol.logical1PulsePair.durationLowLevelPulse.compare(
						pulse.mMicroSecDuration);
				if (log1Compare == TimeRange::IS_WITHIN) {
					result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_1;
				}
			}
			break;
		}
		case PULSE_LEVEL::HI: {
			const bool inverseLevel = protocol.isInverseLevelProtocol();
			const TimeRange::COMPARE_RESULT synchCompare =
					protocol.synchronizationPulsePair.durationHighLevelPulse.compare(pulse.mMicroSecDuration);
			if (inverseLevel) {
				if (synchCompare == TimeRange::IS_WITHIN) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_SECOND_PULSE;
				}
			} else {
				RCSWITCH_ASSERT(protocol.isNormalLevelProtocol());
				if (synchCompare == TimeRange::IS_WITHIN) {
					result.mPulseTypeSynch = PULSE_TYPE::SYNCH_FIRST_PULSE;
				}
			}
			const TimeRange::COMPARE_RESULT log0Compare =
					protocol.logical0PulsePair.durationHighLevelPulse.compare(
					pulse.mMicroSecDuration);
			if (log0Compare == TimeRange::IS_WITHIN) {
				result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_0;
			} else {
				const TimeRange::COMPARE_RESULT log1Compare =
						protocol.logical1PulsePair.durationHighLevelPulse.compare(
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

static inline void collectNormalLevelProtocolCandidates(
		ProtocolCandidates& protocolCandidates, const Pulse&  pulse_0, const Pulse&  pulse_1) {
	const std::pair<const Protocol*, size_t>& protocol = getProtocolTable(NORMAL_LEVEL_PROTOCOLS);
	for(size_t i = 0; i < protocol.second; i++) {
		const Protocol& prot = protocol.first[i];
		if(pulse_0.mMicroSecDuration <
				protocol.first[i].synchronizationPulsePair.durationHighLevelPulse.lowerBound) {
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
	const std::pair<const Protocol*, size_t>& protocol = getProtocolTable(INVERSE_LEVEL_PROTOCOLS);
	for(size_t i = 0; i < protocol.second; i++) {
		const Protocol& prot = protocol.first[i];
		if(pulse_0.mMicroSecDuration <
				protocol.first[i].synchronizationPulsePair.durationLowLevelPulse.lowerBound) {
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
	 const std::pair<const Protocol*, size_t>& protocol = getProtocolTable(mProtocolGroupId);
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
				pulseToPulseTypes(protocol, secondPulse);
		const PulseTypes& pulseTypesFirstPulse =
				pulseToPulseTypes(protocol, firstPulse);

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
