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

#ifndef RCSWITCH_RECEIVER_INTERNAL_RCSWTICH__HPP_
#define RCSWITCH_RECEIVER_INTERNAL_RCSWTICH__HPP_

#include <stddef.h>
#include <stdint.h>

#include "ISR_ATTR.hpp"
#include "Common.hpp"
#include "Container.hpp"
#include "Pulse.hpp"
#include "PulseAnalyzer.hpp"

#define DEBUG_RCSWITCH false

#if DEBUG_RCSWITCH
#include <assert.h>
#define RCSWITCH_ASSERT assert
#else
#define RCSWITCH_ASSERT(expr)
#endif

/** Forward declaration of the class providing the API. */
template<int IOPIN, size_t PULSE_TRACES_COUNT> class RcSwitchReceiver;

namespace RcSwitch {

/**
 * Note: Virtual methods would make sense in some introduced
 * classes, but they are not necessary.
 * Since the code is running in an embedded environment,
 * virtual methods have been avoided on purpose.
 */


/**
 * The type of the value decoded from a received message packet.
 * If the number of data bits of the message packet is bigger than
 * this type can store, trailing data bits are dropped.
 */
typedef uint32_t receivedValue_t;

/**
 * Maximum number of data bits from a message packet that can
 * be stored. If the message packet is bigger, trailing data
 * bits are dropped.
 */
constexpr size_t MAX_MSG_PACKET_BITS     =  8 * sizeof(receivedValue_t);

/**
 * The maximum number of protocols that can be collected.
 *
 * When a synchronization pulse pair is received it can fulfill the
 * policy of multiple protocols. All those protocols are collected
 * and further narrowed down during data phase. I.e. collected protocols
 * that do not match the received data pulses will be dropped.
 * Finally when a message packet has been received, there can be multiple
 * protocols left over. Those can be queried by an API function.
 */
constexpr size_t MAX_PROTOCOL_CANDIDATES =  7;

/**
 * Minimum number of data bits for accepting a message packet
 * to be valid.
 * Can be changed, but must be greater than 0.
 */
constexpr size_t MIN_MSG_PACKET_BITS     =  6;

/**
 * A high level pulse followed by a low level pulse constitute
 * a data bit. For inverse protocols, a low level pulse
 * followed by a high level pulse constitute a data bit.
 * Must not be changed.
 */
constexpr size_t DATA_PULSES_PER_BIT     =  2;


enum class DATA_BIT : int8_t {
	UNKNOWN   =-1,
	LOGICAL_0 = 0,
	LOGICAL_1 = 1,
};

enum PROTOCOL_GROUP_ID {
	UNKNOWN_PROTOCOL       = -1,
	NORMAL_LEVEL_PROTOCOLS  = 0,
	INVERSE_LEVEL_PROTOCOLS = 1,
};

/** A protocol candidate is identified by an index. */
typedef size_t PROTOCOL_CANDIDATE;

/**
 * This container stores the all the protocols that match the
 * synchronization pulses during the synchronization phase.
 */
class ProtocolCandidates : public StackBuffer<PROTOCOL_CANDIDATE, MAX_PROTOCOL_CANDIDATES> {
	using baseClass = StackBuffer<PROTOCOL_CANDIDATE, MAX_PROTOCOL_CANDIDATES>;
	PROTOCOL_GROUP_ID mProtocolGroupId;

public:
	inline ProtocolCandidates() : mProtocolGroupId(UNKNOWN_PROTOCOL) {
	}

	/** Remove all protocol candidates from this container. */
	TEXT_ISR_ATTR_1 inline void reset() {
		baseClass::reset();
		mProtocolGroupId = UNKNOWN_PROTOCOL;
	}

	/**
	 * Push another protocol candidate onto the stack.
	 */
	// Make the base class push() method public.
	using baseClass::push;

	TEXT_ISR_ATTR_2 void setProtocolGroup(const PROTOCOL_GROUP_ID protocolGroup) {
		mProtocolGroupId = protocolGroup;
	}
	TEXT_ISR_ATTR_2 PROTOCOL_GROUP_ID getProtocolGroup() const {
		return mProtocolGroupId;
	}
};


/**
 * This container stores received pulses for debugging and pulse analysis purpose.
 */
template<size_t PULSE_TRACES_COUNT>
class PulseTracer : public RingBuffer<Pulse, PULSE_TRACES_COUNT> {
	using baseClass = RingBuffer<Pulse, PULSE_TRACES_COUNT>;
public:
	static const char* pulseTypeToString(const Pulse& pulse) {
		switch(pulse.mPulseLevel) {
		case PULSE_LEVEL::LO:
			return " LOW";
		case PULSE_LEVEL::HI:
			return "HIGH";
		}
		return "??";
	}

	template<typename T> void dump(T& stream, char separator) const {
		const size_t n = baseClass::size();
		size_t i = 0;
		const size_t indexWidth = digitCount(PULSE_TRACES_COUNT);
		while(i < n) {
			const Pulse& pulse = baseClass::at(i);
			// print trace buffer index
			{
				char buffer[16];
				buffer[0] = ('[');
				sprintUint(&buffer[1], i, indexWidth);
				stream.print(buffer);
				stream.print("]");
			}
			stream.print(separator);
			stream.print(" ");

			// print pulse type (LOW, HIGH)
			stream.print(pulseTypeToString(pulse));
			stream.print(separator);
			stream.print(" ");
			stream.print("for");
			stream.print(separator);
			stream.print(" ");

			// print pulse length
			{
				char buffer[16];
				sprintUint(buffer, pulse.mMicroSecDuration, 6);
				stream.print(buffer);
			}
			stream.print(separator);
			stream.print(" ");
			stream.println("usec");

			++i;
		}
	}

	PulseTracer() {
		/* Initialize pulse tracer elements. */
	}

	/**
	 * Remove all pulses from this pulse tracer container.
	 */
	// Make the base class reset() method public.
	using baseClass::reset;
};

/**
 * This container stores the received data bits of a single message packet
 * sent by the transmitter.
 * If the transmitter sends more data bits than MAX_MSG_PACKET_BITS,
 * the overflow counter of this container will be incremented.
 * The MAX_MSG_PACKET_BITS constant can be increased and the code
 * can be re-compiled to avoid overflows.
 */
class MessagePacket : public StackBuffer<DATA_BIT, MAX_MSG_PACKET_BITS> {
	using baseClass = StackBuffer<DATA_BIT, MAX_MSG_PACKET_BITS>;

public:
	/** Default constructor */
	inline MessagePacket() {
	}

	/**
	 * Remove all data bits from this message packet container.
	 */
	// Make the base class reset() method public.
	using baseClass::reset;
};

/**
 * The receiver is a buffer that holds the last 2 received pulses. It analyzes
 * these last pulses, whenever a new pulse arrives by a new interrupt.
 * When detecting valid synchronization pulse pair the
 * receiver's state changes to DATA_STATE and converts subsequent pulses into
 * data bits that will be added to the message packet buffer.
 * In case of receiving unexpected pulses, the
 * receiver goes back to synch state. When a complete message
 * package has been received the state becomes AVAILABLE until the reset
 * function is called.
 */
class Receiver : public RingBuffer<Pulse, DATA_PULSES_PER_BIT> {
private:
	/** =========================================================================== */
	/** == Privately used types, enumerations, variables and methods ============== */
	using baseClass = RingBuffer<Pulse, DATA_PULSES_PER_BIT>;
	friend class RcSwitch_test;

	/** API class becomes friend. */
	template<int IOPIN, size_t PULSE_TRACES_COUNT> friend class ::RcSwitchReceiver;

	rxTimingSpecTable mRxTimingSpecTableNormal;
	rxTimingSpecTable mRxTimingSpecTableInverse;

	MessagePacket mReceivedMessagePacket;

	volatile bool mMessageAvailable;
	volatile bool mSuspended;

	ProtocolCandidates mProtocolCandidates;
	size_t mDataModePulseCount;

	enum STATE {AVAILABLE_STATE, SYNC_STATE, DATA_STATE};
	enum STATE state() const;

	TEXT_ISR_ATTR_2 rxTimingSpecTable getRxTimingTable(PROTOCOL_GROUP_ID protocolGroup) const;
	TEXT_ISR_ATTR_1 void collectProtocolCandidates(const Pulse&  pulse_0, const Pulse&  pulse_1);
	TEXT_ISR_ATTR_1 void push(uint32_t microSecDuration, const int pinLevel);
	TEXT_ISR_ATTR_1 PULSE_TYPE analyzePulsePair(const Pulse& firstPulse, const Pulse& secondPulse);
	TEXT_ISR_ATTR_1 void retry();

protected:
	uint32_t mMicrosecLastInterruptTime;

	/** ========================================================================== */
	/** ========= Methods used by API class RcSwitchReceiver ===================== */

	/**
	 * Evaluate a new pulse that has been received. Will only be called from within interrupt context.
	 */
	TEXT_ISR_ATTR_0 void handleInterrupt(const int pinLevel, const uint32_t microSecInterruptTime);

	/**
	 * Constructor.
	 */

	Receiver()
		    : mRxTimingSpecTableNormal{nullptr, 0}, mRxTimingSpecTableInverse{nullptr, 0}
		    , mMessageAvailable(false), mSuspended(false)
			, mDataModePulseCount(0), mMicrosecLastInterruptTime(0)	{
	}

private:
	/**
	 * Set the protocol table for receiving data.
	 */
	void setRxTimingSpecTable(const rxTimingSpecTable& rxTimingSpecTable);

	/**
	 * Remove protocol candidates for the mProtocolCandidates buffer.
	 * Remove the all data pulses from this container.
	 * Reset the available flag.
	 *
	 * Will be called from outside of the interrupt handler context.
	 */
	void reset();

public:
	/**
	 * For the following methods, refer to corresponding API class RcSwitchReceiver.
	 */
	inline bool available() const {return state() == AVAILABLE_STATE;}
	receivedValue_t receivedValue() const;
	size_t receivedBitsCount() const;
	inline size_t receivedProtocolCount() const {return mProtocolCandidates.size();}
	int receivedProtocol(const size_t index) const;
	void suspend() {mSuspended = true;}
	void resume() {if(mSuspended) {reset(); mSuspended=false;}}
	size_t getProtcolNumber(const size_t protocolCandidateIndex) const;
	void resetAvailable() {if(available()) {reset();}}

};

template<size_t PULSE_TRACES_COUNT>
class ReceiverWithPulseTracer : public Receiver {
	/** API class becomes friend. */
	template<int IOPIN> friend class ::RcSwitchReceiver;

	/**
	 * The most recent received pulses are stored in the pulse tracer for
	 * debugging purpose.
	 */
	PulseTracer<PULSE_TRACES_COUNT> mPulseTracer;
	volatile mutable bool mPulseTracingLocked = false;

	/** Store a new pulse in the trace buffer of this message packet. */
	TEXT_ISR_ATTR_1 void tracePulse(uint32_t microSecDuration, const int pinLevel) {
		if(not mPulseTracingLocked) {
			Pulse * const currentPulse = mPulseTracer.beyondTop();
			*currentPulse = {microSecDuration, (pinLevel ? PULSE_LEVEL::LO : PULSE_LEVEL::HI)};
			mPulseTracer.selectNext();
		}
	}

	void analyzeTracedPulses(PulseAnalyzer& pulseAnalyzer) const {
		RCSWITCH_ASSERT(mPulseTracingLocked);
		pulseAnalyzer.reset();
		const size_t n = mPulseTracer.size();
		size_t i = 0;
		for(; i < n; i++) {
			pulseAnalyzer.addPulse(mPulseTracer.at(i));
		}
	}

	/** ========================================================================== */
	/** ========= Methods used by API class RcSwitchReceiver ===================== */

	/**
	 * Evaluate a new pulse that has been received. Will only be called from within interrupt context.
	 */
	TEXT_ISR_ATTR_0 inline void handleInterrupt(const int pinLevel, const uint32_t microSecInterruptTime) {
		tracePulse(microSecInterruptTime - mMicrosecLastInterruptTime, pinLevel);
		Receiver::handleInterrupt(pinLevel, microSecInterruptTime);
	}

public:
	/**
	 * For the following methods, refer to corresponding API class RcSwitchReceiver.
	 */
	template <typename T>
	void dumpPulseTracer(T& stream, char separator) const {
		mPulseTracingLocked = true;
		PulseAnalyzer pulseAnalyzer;
		analyzeTracedPulses(pulseAnalyzer);
		mPulseTracer.dump(stream, separator);
		pulseAnalyzer.dump(stream, separator);
		mPulseTracingLocked = false;
	}
};

template<size_t PULSE_TRACES_COUNT> struct ReceiverSelector {
	using receiver_t = ReceiverWithPulseTracer<PULSE_TRACES_COUNT>;

	template<typename T>
	static void dumpPulseTracer(const receiver_t& receiver, T& stream, char separator) {
		receiver.dumpPulseTracer(stream, separator);
	}
};

// Specialize for PULSE_TRACES_COUNT being zero.
template<> struct ReceiverSelector<0> {
	using receiver_t = Receiver;
	template<typename T>
	static void dumpPulseTracer(const receiver_t& receiver, T& stream, char separator) {
		// There are no pulses traced.
	}
};

} /* namespace RcSwitch */

#endif /* RCSWITCH_RECEIVER_INTERNAL_RCSWTICH__HPP_ */
