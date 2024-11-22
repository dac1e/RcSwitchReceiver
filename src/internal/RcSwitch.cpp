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

#include <limits>

#include "ProtocolTimingSpec.hpp"
#include "RcSwitch.hpp"


#if defined max // max macro is not compatible with limits standard library.
#undef max
#endif

#if defined min // min macro is not compatible with limits standard library.
#undef min
#endif

namespace RcSwitch {

#if DEBUG_RCSWITCH

/** Specialize INITIAL_VALUE for DATA_BIT */
template<> inline const DATA_BIT& INITIAL_VALUE<DATA_BIT>() {
	static const DATA_BIT value = DATA_BIT::UNKNOWN;
	return value;
}

/** Specialize INITIAL_VALUE for Pulse */
template<> inline const Pulse& INITIAL_VALUE<Pulse>() {
	static const Pulse value = Pulse{
		0, PULSE_LEVEL::UNKNOWN
	};
	return value;
}

/** Specialize INITIAL_VALUE for PROTOCOL_CANDIDATE */
template<> inline const PROTOCOL_CANDIDATE& INITIAL_VALUE<PROTOCOL_CANDIDATE>() {
	static const PROTOCOL_CANDIDATE value = std::numeric_limits<size_t>::max();
	return value;
}

/** Specialize INITIAL_VALUE for MessagePacket */
template<> inline const MessagePacket& INITIAL_VALUE<MessagePacket>() {
	static const MessagePacket value;
	return value;
}

#endif



static PulseTypes pulseAtoPulseTypes(const RxTimingSpec& protocol, const Pulse &pulse) {
	PulseTypes result = { PULSE_TYPE::UNKNOWN, PULSE_TYPE::UNKNOWN };
	{
		const TimeRange::COMPARE_RESULT synchCompare =
				protocol.synchronizationPulsePair.durationA.compare(pulse.mMicroSecDuration);

		/* First synch pulse is allowed to be longer */
		if (synchCompare != TimeRange::TOO_SHORT) {
			result.mPulseTypeSynch = PULSE_TYPE::SYNCH_FIRST_PULSE;
		}
	}

	{
		const TimeRange::COMPARE_RESULT log0Compare =
				protocol.data0pulsePair.durationA.compare(pulse.mMicroSecDuration);

		if (log0Compare == TimeRange::IS_WITHIN) {
			result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_0;
		} else {
			const TimeRange::COMPARE_RESULT log1Compare =
					protocol.data1pulsePair.durationA.compare(pulse.mMicroSecDuration);
			if (log1Compare == TimeRange::IS_WITHIN) {
				result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_1;
			}
		}
	}
	return result;
}

static PulseTypes pulseBtoPulseTypes(const RxTimingSpec& protocol, const Pulse &pulse) {
	PulseTypes result = { PULSE_TYPE::UNKNOWN, PULSE_TYPE::UNKNOWN };
	{
		const TimeRange::COMPARE_RESULT synchCompare =
				protocol.synchronizationPulsePair.durationB.compare(pulse.mMicroSecDuration);

		/* First synch pulse is allowed to be longer */
		if (synchCompare == TimeRange::IS_WITHIN) {
			result.mPulseTypeSynch = PULSE_TYPE::SYNCH_SECOND_PULSE;
		}
	}

	{
		const TimeRange::COMPARE_RESULT log0Compare =
				protocol.data0pulsePair.durationB.compare(pulse.mMicroSecDuration);

		if (log0Compare == TimeRange::IS_WITHIN) {
			result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_0;
		} else {
			const TimeRange::COMPARE_RESULT log1Compare =
					protocol.data1pulsePair.durationB.compare(
					pulse.mMicroSecDuration);
			if (log1Compare == TimeRange::IS_WITHIN) {
				result.mPulseTypeData = PULSE_TYPE::DATA_LOGICAL_1;
			}
		}
	}
	return result;
}

static inline void collectProtocolCandidates(const std::pair<const RxTimingSpec*, size_t>& protocol,
		ProtocolCandidates& protocolCandidates, const Pulse&  pulseA, const Pulse&  pulseB) {
	for(size_t i = 0; i < protocol.second; i++) {
		const RxTimingSpec& prot = protocol.first[i];
		if(pulseA.mMicroSecDuration <
				protocol.first[i].synchronizationPulsePair.durationA.lowerBound) {
			/* Protocols are sorted in ascending order of pulseA.mMicroSecDuration
			 * So further protocols will have even higher mMicroSecDuration. Hence we can
			 * break here immediately.
			 */
			return;
		}

		if(pulseA.mMicroSecDuration <
				prot.synchronizationPulsePair.durationA.upperBound) {
			if(pulseB.mMicroSecDuration >=
					prot.synchronizationPulsePair.durationB.lowerBound) {
				if(pulseB.mMicroSecDuration <
						prot.synchronizationPulsePair.durationB.upperBound) {
					protocolCandidates.push(i);
				}
			}
		}
	}
}

// ======== ProtocolCandidates =========
size_t Receiver::getProtcolNumber(const size_t protocolCandidateIndex) const {
	 const std::pair<const RxTimingSpec*, size_t>& protocol = getRxTimingTable(mProtocolCandidates.getProtocolGroup());
	 RCSWITCH_ASSERT(protocolCandidateIndex < size());
	 const size_t protocolIndex = mProtocolCandidates.at(protocolCandidateIndex);
	 RCSWITCH_ASSERT(protocolIndex < protocol.second);
	 return protocol.first[protocolIndex].protocolNumber;
}

// ======== Receiver ===================
void Receiver::collectProtocolCandidates(const Pulse&  pulse_0, const Pulse&  pulse_1) {
  if(pulse_0.mPulseLevel != pulse_1.mPulseLevel) {
		if(pulse_0.mPulseLevel == PULSE_LEVEL::HI) {
			mProtocolCandidates.setProtocolGroup(NORMAL_LEVEL_PROTOCOLS);
			RcSwitch::collectProtocolCandidates(getRxTimingTable(NORMAL_LEVEL_PROTOCOLS), mProtocolCandidates, pulse_0, pulse_1);
		} else if(pulse_0.mPulseLevel == PULSE_LEVEL::LO) {
			mProtocolCandidates.setProtocolGroup(INVERSE_LEVEL_PROTOCOLS);
			RcSwitch::collectProtocolCandidates(getRxTimingTable(INVERSE_LEVEL_PROTOCOLS), mProtocolCandidates, pulse_0, pulse_1);
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
inline PULSE_TYPE Receiver::analyzePulsePair(const Pulse& pulseA, const Pulse& pulseB) {
	PULSE_TYPE result = PULSE_TYPE::UNKNOWN;
	const std::pair<const RxTimingSpec*, size_t> protocols = getRxTimingTable(mProtocolCandidates.getProtocolGroup());
	size_t protocolCandidatesIndex = mProtocolCandidates.size();
	while(protocolCandidatesIndex > 0) {
		--protocolCandidatesIndex;
		RCSWITCH_ASSERT(protocolCandidatesIndex < protocols.second);
		const RxTimingSpec& protocol = protocols.first[mProtocolCandidates[protocolCandidatesIndex]];

		const PulseTypes& pulseTypesPulseA = pulseAtoPulseTypes(protocol, pulseA);
		const PulseTypes& pulseTypesPulseB = pulseBtoPulseTypes(protocol, pulseB);

		if(pulseTypesPulseB.mPulseTypeSynch == PULSE_TYPE::SYNCH_SECOND_PULSE
				&& pulseTypesPulseA.mPulseTypeSynch == PULSE_TYPE::SYNCH_FIRST_PULSE) {
			/* The pulses match the protocol for synch pulses. */
			return PULSE_TYPE::SYCH_PULSE;
		}

		if(pulseTypesPulseB.mPulseTypeData == pulseTypesPulseA.mPulseTypeData
				&& pulseTypesPulseB.mPulseTypeData !=  PULSE_TYPE::UNKNOWN) {
			/* The pulses match the protocol for data pulses */
			if(result == PULSE_TYPE::UNKNOWN) { /* keep the first match */
				result = pulseTypesPulseB.mPulseTypeData;
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
					const Pulse& pulseA = at(size()-2);
					const Pulse& pulseB = at(size()-1);

					collectProtocolCandidates(pulseA, pulseB);
					/* If the above call has identified any valid protocol
					 * candidate, the state has implicitly become DATA_STATE.
					 * Refer to function state(). */
				}
				break;
			case DATA_STATE:
				if(++mDataModePulseCount == 2) {
					mDataModePulseCount = 0;
					const Pulse& pulseA = at(size()-2);
					const Pulse& pulseB = at(size()-1);
					const PULSE_TYPE pulseType = analyzePulsePair(pulseA, pulseB);
					if(pulseType == PULSE_TYPE::UNKNOWN) {
						/* Unknown pulses received, hence start from scratch. Current pulses
						 * might be the synch start, but for a different protocol. */
						mProtocolCandidates.reset();
						/* Check current pulses for being a synch of a different protocol. */
						collectProtocolCandidates(pulseA, pulseB);
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
								collectProtocolCandidates(pulseA, pulseB);
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
		return getProtcolNumber(index);
	}
	return -1;
}

std::pair<const RxTimingSpec*, size_t> Receiver::getRxTimingTable(
		PROTOCOL_GROUP_ID protocolGroup) const {
	switch (protocolGroup) {
	case PROTOCOL_GROUP_ID::INVERSE_LEVEL_PROTOCOLS:
		return mRxTimingSpecTableNormal;
		break;
	case PROTOCOL_GROUP_ID::NORMAL_LEVEL_PROTOCOLS:
		return mRxTimingSpecTableInverse;
		break;
	case PROTOCOL_GROUP_ID::UNKNOWN_PROTOCOL:
		RCSWITCH_ASSERT(false);
		break;
	}
	return std::make_pair(nullptr, 0);
}

void Receiver::setRxProtocolTable(const RxTimingSpec *rxTimingSpecTable,
		size_t tableLength) {
	size_t i = 0;
	for (; i < tableLength; i++) {
		if (rxTimingSpecTable->bInverseLevel) {
			break;
		}
	}
	mRxTimingSpecTableNormal.first = &rxTimingSpecTable[0];
	mRxTimingSpecTableNormal.second = i;
	mRxTimingSpecTableInverse.first = &rxTimingSpecTable[i];
	mRxTimingSpecTableInverse.second = tableLength - i;
}

} /* namespace RcSwitch */

#include <UartClass.h>

namespace RcSwitch {

void Receiver::dumpRxTimingTable(UARTClass& serial, PROTOCOL_GROUP_ID protocolGroup) {
	if(protocolGroup == PROTOCOL_GROUP_ID::NORMAL_LEVEL_PROTOCOLS ||
	   protocolGroup == PROTOCOL_GROUP_ID::INVERSE_LEVEL_PROTOCOLS) {

		const std::pair<const RxTimingSpec*, size_t> timingTable =
				protocolGroup == PROTOCOL_GROUP_ID::NORMAL_LEVEL_PROTOCOLS ?
						mRxTimingSpecTableNormal : mRxTimingSpecTableInverse;

		for (size_t i = 0; i < timingTable.second; i++) {
			const RxTimingSpec& p = timingTable.first[i];

			if(p.protocolNumber < 10) {
				serial.print(' ');
			}
			serial.print(p.protocolNumber);

			if(not p.bInverseLevel) {
				serial.print(", normal");
			} else {
				serial.print(",inverse");
			}
			serial.print(",SY:[");
			serial.print(p.synchronizationPulsePair.durationA.lowerBound);
			serial.print("..");
			serial.print(p.synchronizationPulsePair.durationA.upperBound);
			serial.print("],[");
			serial.print(p.synchronizationPulsePair.durationB.lowerBound);
			serial.print("..");
			serial.print(p.synchronizationPulsePair.durationB.upperBound);
			serial.print("],D0:[");
			serial.print(p.data0pulsePair.durationA.lowerBound);
			serial.print("..");
			serial.print(p.data0pulsePair.durationA.upperBound);
			serial.print("],[");
			serial.print(p.data0pulsePair.durationB.lowerBound);
			serial.print("..");
			serial.print(p.data0pulsePair.durationB.upperBound);
			serial.print("],D1:[");
			serial.print(p.data1pulsePair.durationA.lowerBound);
			serial.print("..");
			serial.print(p.data1pulsePair.durationA.upperBound);
			serial.print("],[");
			serial.print(p.data1pulsePair.durationB.lowerBound);
			serial.print("..");
			serial.print(p.data1pulsePair.durationB.upperBound);
			serial.println("]");
		}
	}
}

} /* namespace RcSwitch */
