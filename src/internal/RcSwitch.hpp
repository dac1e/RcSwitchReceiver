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

#define DEBUG_RCSWITCH false

#if DEBUG_RCSWITCH
#include <assert.h>
#endif

#include "internal/Common.hpp"

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
 * Setting DEBUG_RCSWITCH to true will:
 *
 * 1) Initialize elements of arrays within objects upon
 * reset() calls. This helps to quickly recognize
 * that an array element was explicitly set.
 *
 * 2) Map macro RCSWITCH_ASSERT to the system function
 * assert.
 */

#if DEBUG_RCSWITCH
#define RCSWITCH_ASSERT assert
#else
#define RCSWITCH_ASSERT(expr)
#endif


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
 * Maximum number of data bits from a message packet that can
 * be stored. If the message packet is bigger, trailing data
 * bits are dropped.
 * Can be changed, but must be greater than 0.
 */
constexpr size_t MAX_MSG_PACKET_BITS     = 32;

/**
 * A high level pulse followed by a low level pulse constitute
 * a data bit. For inverse protocols, a low level pulse
 * followed by a high level pulse constitute a data bit.
 * Must not be changed.
 */
constexpr size_t DATA_PULSES_PER_BIT     =  2;



#if DEBUG_RCSWITCH
/**
 * A template function structure providing initial values for
 * the particular types. Will be specialized for the types where
 * initial value is needed. */
template<typename ELEMENT_TYPE> struct INITIAL_VALUE;

/** Specialize INITIAL_VALUE for size_t */
template<> struct INITIAL_VALUE<size_t> {
	static constexpr size_t value = static_cast<size_t>(-1);
};

#endif

/**
 * A container that encapsulates fixed size arrays.
 */
template<typename ELEMENT_TYPE, size_t CAPACITY>
class Array {
protected:
	typedef ELEMENT_TYPE element_type;
	/** The array where data is stored. */
	element_type mData[CAPACITY];

	/** A variable to store the actual size of the array. */
	size_t mSize;

	TEXT_ISR_ATTR_1 inline void init() {
#if DEBUG_RCSWITCH // Initialize only if debugging is enabled
		size_t i = 0;
		for(; i < CAPACITY; i++) {
			mData[i] = INITIAL_VALUE<ELEMENT_TYPE>::value;
		}
#endif
	}

	TEXT_ISR_ATTR_1 inline void reset() {
		mSize = 0;
		init();
	}

	/** Default constructor */
	Array() : mSize(0) {
		init();
	}
public:
	TEXT_ISR_ATTR_1 inline size_t size() const {return mSize;}
	TEXT_ISR_ATTR_1 inline bool canGrow() const {return mSize < CAPACITY;}
};

/**
 * A container that encapsulates a fixed size stack. Elements can be
 * pushed onto the stack as long as the actual size is smaller than
 * the capacity. Otherwise, the pushed element is dropped, and the
 * overflow counter is incremented.
 */
template<typename ELEMENT_TYPE, size_t CAPACITY>
class StackBuffer : public Array<ELEMENT_TYPE, CAPACITY> {
	friend class RcSwitch_test;
protected:
	using baseClass = Array<ELEMENT_TYPE, CAPACITY>;
	using element_type = typename baseClass::element_type;
	/**
	 * A counter that will be incremented, when an element couldn't be pushed,
	 * because this stack was already full. */
	uint32_t mOverflow;

	/* Set the actual size of this stack to zero and clear
	 * the overflow counter.
	 */
	TEXT_ISR_ATTR_2 inline void reset() {baseClass::reset(), mOverflow = 0;}

	/* Default constructor */
	inline StackBuffer() : mOverflow(0) {}
public:
	/** Make the capacity template argument available as const expression. */
	static constexpr size_t capacity = CAPACITY;

	/**
	 * Return a pointer to the memory, that stores the element
	 * beyond the top stack element. Return null, if there is
	 * no beyond top stack element available.
	 */
	TEXT_ISR_ATTR_2 inline element_type* beyondTop() {
		if (baseClass::mSize < CAPACITY) {
			return &baseClass::mData[baseClass::mSize];
		}
		return nullptr;
	}

	/**
	 * Make the beyond top stack element to the top one, if the
	 * stack has still space to take a new element. Otherwise
	 * set increment the overflow counter.
	 * Returns true if successful, otherwise false.
	 */
	TEXT_ISR_ATTR_2 inline bool selectNext() {
		if (baseClass::mSize < CAPACITY) {
			++baseClass::mSize;
			return true;
		}
		++mOverflow;
		return false;
	}

	/**
	 * Push an element on top of the stack.
	 * Returns true if successful, otherwise false.
	 */
	TEXT_ISR_ATTR_1 bool push(const element_type &value) {
		element_type* const storage = beyondTop();
		if (storage) {
			*storage = value;
		}
		return selectNext();
	}

	/**
	 * Return a const reference to the element at the specified index.
	 * The index is validated by assert() system function.
	 */
	inline const element_type& at(const size_t index) const {
		RCSWITCH_ASSERT(index < baseClass::mSize);
		return baseClass::mData[index];
	}

	/* Refer to method at() */
	inline const element_type& operator[](const size_t index) const {
		return at(index);
	}

	/**
	 * Remove a stack element at the specified index.
	 * Important: This method may invalidate the element that was
	 * previously obtained by the at() function respectively the
	 * operator[]. The function will alter the actual stack size.
	 * The overflow counter stays untouched.
	 */
	TEXT_ISR_ATTR_2 void remove(const size_t index) {
		if(index < baseClass::mSize) {
#if DEBUG_RCSWITCH // Initialize only if debugging support is enabled
			baseClass::mData[index] = INITIAL_VALUE<element_type>::value;
#endif
			size_t i = index+1;
			for(;i < baseClass::mSize; i++) {
				baseClass::mData[i-1] = baseClass::mData[i];
			}
			--baseClass::mSize;
#if DEBUG_RCSWITCH // Initialize only if debugging support is enabled
			baseClass::mData[baseClass::mSize] = INITIAL_VALUE<element_type>::value;
#endif
		}
	}

	/* Return the value of the overflow counter. */
	inline size_t overflowCount()const {return mOverflow;}
};

/**
 * A container that encapsulates a fixed size stack. Elements can be
 * pushed onto the stack. When the size of the stack has reached the
 * capacity, the bottom element will be dropped on cost of the new
 * pushed element.
 */
template<typename ELEMENT_TYPE, size_t CAPACITY>
class RingBuffer : public Array<ELEMENT_TYPE, CAPACITY> {
	friend class RcSwitch_test;
	/**
	 * The index of the bottom element of the ring buffer.
	 */
	size_t mBegin;
	TEXT_ISR_ATTR_2 static size_t inline squashedIndex(const size_t i)
	{
		return (i + CAPACITY) % CAPACITY;
	}
protected:
	using baseClass = Array<ELEMENT_TYPE, CAPACITY>;
	using element_type = typename baseClass::element_type;

	/** Set the actual size of this ring buffer to zero. */
	TEXT_ISR_ATTR_2 inline void reset() {baseClass::reset(); mBegin = 0;}

	/** Default constructor */
	inline RingBuffer() : mBegin(0) {}
public:
	static constexpr size_t capacity = CAPACITY;

	/**
	 * Return a pointer to the memory, that stores the element
	 * beyond the top ring buffer element.
	 */
	TEXT_ISR_ATTR_2 inline element_type* beyondTop() {
		const size_t index = squashedIndex(mBegin + baseClass::mSize);
		return &baseClass::mData[index];
	}

	/**
	 * Make the beyond top ring buffer element to the top element. If
	 * the ring buffer size has already reached the capacity, the
	 * bottom element will be dropped.
	 */
	TEXT_ISR_ATTR_2 inline void selectNext() {
		if(baseClass::mSize < capacity) {
			++baseClass::mSize;
		} else {
			mBegin = squashedIndex(mBegin +  baseClass::mSize + 1);
		}
	}

	void push(const element_type &value) {
		element_type* const storage = beyondTop();
		*storage = value;
		selectNext();
	}

	/**
	 * Return a const reference to the element at the specified index.
	 * The index is validated by assert() system function.
	 */
	TEXT_ISR_ATTR_1 inline const element_type& at(const size_t index) const {
		RCSWITCH_ASSERT(index < baseClass::mSize);
		return baseClass::mData[squashedIndex(mBegin + index)];
	}

	/**
	 * Return a reference to the element at the specified index.
	 * The index is validated by assert() system function.
	 */
	TEXT_ISR_ATTR_1 inline element_type& at(const size_t index) {
		RCSWITCH_ASSERT(index < baseClass::mSize);
		return baseClass::mData[squashedIndex(mBegin + index)];
	}
};

enum class DATA_BIT {
	UNKNOWN   =-1,
	LOGICAL_0 = 0,
	LOGICAL_1 = 1,
};

#if DEBUG_RCSWITCH
/** Specialize INITIAL_VALUE for DATA_BIT */
template<> struct INITIAL_VALUE<DATA_BIT> {
	static constexpr DATA_BIT value = DATA_BIT::UNKNOWN;
};
#endif

enum class PULSE_LEVEL : uint8_t {
	UNKNOWN = 0,
	LO,
	HI,
};

enum class PULSE_TYPE : uint8_t {
	UNKNOWN = 0,
	SYCH_PULSE,
	SYNCH_FIRST_PULSE,
	SYNCH_SECOND_PULSE,
	DATA_LOGICAL_0,
	DATA_LOGICAL_1,
};

struct PulseTypes {
	PULSE_TYPE mPulseTypeSynch;
	PULSE_TYPE mPulseTypeData;
};

struct Pulse {
	uint32_t    mMicroSecDuration;
	PULSE_LEVEL mPulseLevel;
};

#if DEBUG_RCSWITCH
/** Specialize INITIAL_VALUE for Pulse */
template<> struct INITIAL_VALUE<Pulse> {
	static constexpr Pulse value = Pulse{0, PULSE_LEVEL::UNKNOWN};
};
#endif


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
		/** Initialize protocol candidates elements. */
		Array::init();
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
 * This container stores received pulses for debugging purpose. The
 * purpose is to view the pulses in a debugging session. */
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

	template<typename T> void dumpBackward(T& s) {
		const size_t n = baseClass::size();
		size_t i = 0;
		const size_t indexWidth = digitCount(PULSE_TRACES_COUNT);
		while(i < n) {
			++i;
			const Pulse& pulse = baseClass::at(n-i);

			{	// print index
				char buffer[16];
				size_t b = 0;
				buffer[b++] = ('[');
				sprintUint(&buffer[b++], i, indexWidth);
				s.print(buffer);
				s.print("] ");
			}

			s.print(pulseTypeToString(pulse));
			s.print(" after ");

			{ // print pulse length
				char buffer[16];
				sprintUint(buffer, pulse.mMicroSecDuration, 6);
				s.print(buffer);
			}

			s.println(" usec");
		}
	}

	PulseTracer() {
		/* Initialize pulse tracer elements. */
		Array<Pulse, PULSE_TRACES_COUNT>::init();
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
		/* Initialize data bit elements. */
		Array::init();
	}

	/**
	 * Remove all data bits from this message packet container.
	 */
	// Make the base class reset() method public.
	using baseClass::reset;
};

using RcSwitch::rxTimingSpecTable;

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
	/**
	 * Evaluate a new pulse that has been received. Will only be called from within interrupt context.
	 */
	TEXT_ISR_ATTR_0 void handleInterrupt(const int pinLevel, const uint32_t microSecInterruptTime);

	/** ========================================================================== */
	/** ========= Methods used by API class RcSwitchReceiver ===================== */

	/**
	 * Constructor.
	 */

	Receiver()
		    : mRxTimingSpecTableNormal{nullptr, 0}, mRxTimingSpecTableInverse{nullptr, 0}
		    , mMessageAvailable(false), mSuspended(false)
			, mDataModePulseCount(0), mMicrosecLastInterruptTime(0)
			{
			/* Initialize pulse elements. */
			Array::init();
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

	/**
	 * For the following methods, refer to corresponding API class RcSwitchReceiver.
	 */
	inline bool available() const {return state() == AVAILABLE_STATE;}
	uint32_t receivedValue() const;
	size_t receivedBitsCount() const;
	inline size_t receivedProtocolCount() const {return mProtocolCandidates.size();}
	int receivedProtocol(const size_t index) const;
	void suspend() {mSuspended = true;}
	void resume() {if(mSuspended) {reset(); mSuspended=false;}}
	size_t getProtcolNumber(const size_t protocolCandidateIndex) const;
	template <typename T> void dumpPulseTracer(T& serial) {/* do nothing. */}
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
	volatile bool mPulseTracerDumping = false;

	/**
	 * Evaluate a new pulse that has been received. Will only be called from within interrupt context.
	 */
	TEXT_ISR_ATTR_0 inline void handleInterrupt(const int pinLevel, const uint32_t microSecInterruptTime) {
		tracePulse(microSecInterruptTime - mMicrosecLastInterruptTime, pinLevel);
		Receiver::handleInterrupt(pinLevel, microSecInterruptTime);
	}

	/** Store a new pulse in the trace buffer of this message packet. */
	TEXT_ISR_ATTR_1 void tracePulse(uint32_t microSecDuration, const int pinLevel) {
		if(not mPulseTracerDumping) {
			Pulse * const currentPulse = mPulseTracer.beyondTop();
			*currentPulse = {microSecDuration, (pinLevel ? PULSE_LEVEL::LO : PULSE_LEVEL::HI)};
			mPulseTracer.selectNext();
		}
	}

	/**
	 * For the following methods, refer to corresponding API class RcSwitchReceiver.
	 */
	template <typename T> void dumpPulseTracer(T& serial) {
		mPulseTracerDumping = true;
		mPulseTracer.dumpBackward(serial);
		mPulseTracerDumping = false;
	}
};

template<size_t PULSE_TRACES_COUNT> struct ReceiverSelector {
	using receiver_t = ReceiverWithPulseTracer<PULSE_TRACES_COUNT>;
};

template<> struct ReceiverSelector<0> {
	using receiver_t = Receiver;
};

} /* namespace RcSwitch */

#endif /* RCSWITCH_RECEIVER_INTERNAL_RCSWTICH__HPP_ */
