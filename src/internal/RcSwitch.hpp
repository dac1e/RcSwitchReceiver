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
#include "RxTimingSpecTable.hpp"
#include "RcSwitchContainer.hpp"
#include "Pulse.hpp"
#include "PulseTracer.hpp"
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
 * The type of the value decoded from a received message packet.
 * If the number of data bits of the message packet is bigger
 * than this type can store, trailing data bits are dropped.
 */
typedef uint32_t receivedValue_t;

/**
 * Maximum number of data bits from a message packet that can
 * be stored. If the message packet is bigger, trailing data
 * bits are dropped.
 */
constexpr size_t MAX_MSG_PACKET_BITS = 8 * sizeof(receivedValue_t);

/**
 * The maximum number of protocols that can be collected.
 *
 * When a synchronization pulse pair is received it can fulfill the
 * policy of multiple protocols. All those protocols are collected
 * and further narrowed down during data phase. I.e. collected
 * protocols that do not match the received data pulses will be
 * dropped.  Finally when a message packet has been received, there
 * can be multiple protocols left over.
 */
constexpr size_t MAX_PROTOCOL_CANDIDATES = 7;

/**
 * Minimum number of data bits for accepting a message packet
 * to be valid.
 */
constexpr size_t MIN_MSG_PACKET_BITS = 6;

/**
 * A high level pulse followed by a low level pulse constitute
 * a data bit. For inverse protocols, a low level pulse
 * followed by a high level pulse constitute a data bit.
 */
constexpr size_t DATA_PULSES_PER_BIT = 2;


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
	TEXT_ISR_ATTR_1_INLINE void reset();

	/**
	 * Push another protocol candidate onto the stack.
	 */
	using baseClass::push; // Just make the base class push() method public.

	TEXT_ISR_ATTR_2 void setProtocolGroup(const PROTOCOL_GROUP_ID protocolGroup) {
		mProtocolGroupId = protocolGroup;
	}
	TEXT_ISR_ATTR_2 PROTOCOL_GROUP_ID getProtocolGroup() const {
		return mProtocolGroupId;
	}
};

/**
 * This container stores the received data bits of a single message packet.
 * If the transmitter sends more data bits than MAX_MSG_PACKET_BITS,
 * the overflow counter of this container will be incremented.
 */
class MessagePacket : public StackBuffer<DATA_BIT, MAX_MSG_PACKET_BITS> {
	using baseClass = StackBuffer<DATA_BIT, MAX_MSG_PACKET_BITS>;

public:
	/** Default constructor */
	inline MessagePacket() {}

	/**
	 * Remove all data bits from this message packet container.
	 */
	// Make the base class reset() method public.
	using baseClass::reset;
};

/**
 * The receiver is a buffer that holds the last 2 received pulses.
 * It analyzes these last pulses, whenever a new pulse arrives.
 * When detecting valid synchronization pulse pair the receiver's
 * state changes to DATA_STATE and converts subsequent pulses into
 * data bits that will be added to the message packet buffer.
 * In case of receiving unexpected pulses, the receiver goes back
 * to the synch state. When a complete message package has been
 * received the state becomes AVAILABLE until the reset function
 * is called.
 */
class Receiver : public RingBuffer<Pulse, DATA_PULSES_PER_BIT> {
private:
	/** =========================================================================== */
	/** == Privately used types, enumerations, variables and methods ============== */
	using baseClass = RingBuffer<Pulse, DATA_PULSES_PER_BIT>;
	friend class RcSwitch_test;

	/** API class becomes friend. */
	template<int IOPIN, size_t PULSE_TRACES_COUNT> friend class ::RcSwitchReceiver;

	RxTimingSpecTable mRxTimingSpecTableNormal;
	RxTimingSpecTable mRxTimingSpecTableInverse;

	MessagePacket mReceivedMessagePacket;

	volatile bool mMessageAvailable;
	volatile bool mSuspended;

	ProtocolCandidates mProtocolCandidates;
	size_t mDataModePulseCount;

	enum STATE {AVAILABLE_STATE, SYNC_STATE, DATA_STATE};
	enum STATE state() const;

	TEXT_ISR_ATTR_2 RxTimingSpecTable getRxTimingTable(PROTOCOL_GROUP_ID protocolGroup) const;
	TEXT_ISR_ATTR_1 void collectProtocolCandidates(const Pulse&  pulse_0, const Pulse&  pulse_1);
	TEXT_ISR_ATTR_1 void push(uint32_t usecDuration, const int pinLevel);
	TEXT_ISR_ATTR_1 PULSE_TYPE analyzePulsePair(const Pulse& firstPulse, const Pulse& secondPulse);
	TEXT_ISR_ATTR_1 void retry();

protected:
	uint32_t mUsecLastInterrupt;

	/** ========================================================================== */
	/** ========= Methods used by API class RcSwitchReceiver ===================== */

	/**
	 * Evaluate a new pulse that has been received.
	 * Will only be called from within interrupt context.
	 */
	TEXT_ISR_ATTR_0 void handleInterrupt(const int pinLevel, const uint32_t usecInterruptEntry);

	/**
	 * Default constructor.
	 */
	Receiver()
		    : mRxTimingSpecTableNormal{nullptr, 0}, mRxTimingSpecTableInverse{nullptr, 0}
		    , mMessageAvailable(false), mSuspended(false)
			, mDataModePulseCount(0), mUsecLastInterrupt(0)	{
	}

private:
	/**
	 * Set the protocol table for receiving data.
	 */
	void setRxTimingSpecTable(const RxTimingSpecTable& rxTimingSpecTable);

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
	unsigned int getProtcolNumber(const size_t protocolCandidateIndex) const;
	void resetAvailable() {if(available()) {reset();}}

};

/**
 * Just delegate to ::micros(). The reason is to avoid include of "Arduino.h" within this file.
 */
TEXT_ISR_ATTR_1 uint32_t micros_();


template<size_t PULSE_TRACES_COUNT>
class ReceiverWithPulseTracer : public Receiver {
	/** API class becomes friend. */
	template<int IOPIN> friend class ::RcSwitchReceiver;

	/**
	 * The most recent received pulses are stored in the pulse tracer for
	 * analyzing purpose.
	 */
	PulseTracer<PULSE_TRACES_COUNT> mPulseTracer;
	volatile mutable bool mPulseTracingLocked = false;

	/** Store a new pulse in the trace buffer of this message packet. */
	TEXT_ISR_ATTR_1 void tracePulse(const uint32_t usecInterruptEntry, const int pinLevel, const uint32_t usecLastInterrupt) {
		if(not mPulseTracingLocked) {
			TraceRecord * const traceRecord = mPulseTracer.beyondTop();
			const uint32_t usecPulseDuration = usecInterruptEntry - usecLastInterrupt;
			const uint32_t usecInteruptDuration = micros_() - usecInterruptEntry;
			const PULSE_LEVEL pulseLevel = pinLevel ? PULSE_LEVEL::LO : PULSE_LEVEL::HI;
			traceRecord->set(usecPulseDuration, pulseLevel, usecInteruptDuration);
			mPulseTracer.selectNext();
		}
	}

	/** ========================================================================== */
	/** ========= Methods used by API class RcSwitchReceiver ===================== */

	/**
	 * Evaluate a new pulse that has been received. Will
	 * only be called from within interrupt context.
	 */
	TEXT_ISR_ATTR_0_INLINE void handleInterrupt(const int pinLevel, const uint32_t usecInterruptEntry);

public:
	/**
	 * For the following methods, refer to corresponding API
	 * class RcSwitchReceiver.
	 */
	template <typename T>
	void dumpAndDedcucePulses(T& stream, const char* separator, bool bDumpPulses, bool bDeduceProtocol) const {
		mPulseTracingLocked = true;
		if(bDumpPulses) {
			stream.println("\n==== Dumping traced pulses: ==== ");
			mPulseTracer.dump(stream, separator);
			stream.println("==== done!                 ===== ");
		}
		if(bDeduceProtocol){
			const RingBufferReadAccess<TraceRecord> readAccess(mPulseTracer);
			PulseAnalyzer pulseAnalyzer(readAccess);
			stream.println("\n==== Deducing RC protocol: ===== ");
			pulseAnalyzer.dedcuceProtocol();
			pulseAnalyzer.dump(stream, separator);
			stream.println("==== done!                 ===== ");
		}
		mPulseTracingLocked = false;
	}
};

template<size_t PULSE_TRACES_COUNT>
void ReceiverWithPulseTracer<PULSE_TRACES_COUNT>::handleInterrupt(const int pinLevel, const uint32_t usecInterruptEntry) {
	const uint32_t usecLastInterrupt = mUsecLastInterrupt;
	Receiver::handleInterrupt(pinLevel, usecInterruptEntry);
	tracePulse(usecInterruptEntry, pinLevel, usecLastInterrupt);
}

static constexpr size_t MIN_PULSE_TRACES_FOR_PROTOCOL_DEDUCTION = 132;
static const char* const toLessPulseTracesError =
	"The PULSE_TRACES_COUNT parameter of your RcSwitchReceiver is too "
	"low for protocol deduction. Please change to 132 or higher.";

static const char* const noPulsesToTraceError = "The PULSE_TRACES_COUNT "
	"parameter is 0. There are no pulses to dump.";

/**
 * ReceiverSelector is used to select a Receiver class as receiver_t,
 * depending on the PULSE_TRACES_COUNT. This default implementation
 * will select class ReceiverWithPulseTracer<PULSE_TRACES_COUNT> as
 * receiver_t.
 */
template<size_t PULSE_TRACES_COUNT> struct ReceiverSelector {
	using receiver_t = ReceiverWithPulseTracer<PULSE_TRACES_COUNT>;

	template<typename T>
	static void dumpPulseTracer(const receiver_t& receiver, T& stream, const char* separator) {
		receiver.dumpAndDedcucePulses(stream, separator, true, false);
	}

	template<typename T>
	static void deduceProtocolFromPulseTracer(const receiver_t& receiver, T& stream) {
		if(PULSE_TRACES_COUNT < MIN_PULSE_TRACES_FOR_PROTOCOL_DEDUCTION) {
			stream.println(toLessPulseTracesError);
		} else {
			receiver.dumpAndDedcucePulses(stream, "", false, true);
		}
	}
};

/**
 * Specialize ReceiverSelector for PULSE_TRACES_COUNT being zero.
 * This implementation will select class Receiver as receiver_t.
 * Class Receiver does not have a mPulseTracer member. Hence
 * this will save some memory.
 */
template<> struct ReceiverSelector<0> {
	using receiver_t = Receiver;

	template<typename T>
	static void dumpPulseTracer(const receiver_t& receiver, T& stream, const char* separator) {
		stream.println(noPulsesToTraceError);
	}

	template<typename T>
	static void deduceProtocolFromPulseTracer(const receiver_t& receiver, T& stream) {
		stream.println(toLessPulseTracesError);
	}
};

} /* namespace RcSwitch */

#if not defined(ESP32) && not defined(ESP8266)
#include "RcSwitch.inc"
#endif

#endif /* RCSWITCH_RECEIVER_INTERNAL_RCSWTICH__HPP_ */
