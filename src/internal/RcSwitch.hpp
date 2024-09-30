/*
 * RcSwtichReceiver.hpp
 *
 *  Created on: 11.09.2024
 *      Author: Wolfgang
 */
#pragma once

#ifndef _RCSWTICH_INTERNAL_HPP_
#define RCSWTICH_INTERNAL_HPP_

#include <stdint.h>
#include <sys/types.h>
#include <algorithm>

/** Forward declaration of the class providing the API. */
template<int IOPIN> class RcSwitchReceiver;

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
 * 1) Enable a pulse tracer that stores the last N pulses
 * that were processed by the interrupt handler.
 * For the value of N refer to constexpr MAX_PULSE_TRACES.
 * The contents of the pulse tracer can be viewed in a
 * debugger. JTAG debugging is supported on some
 * arduino variants like arduino due.
 *
 * 2) Initialize elements of arrays within objects upon
 * reset() calls. This helps to quickly recognize
 * that an array element was explicitly set to a
 * valid value during the synch or data receive phase.
 *
 * 3) Maps macro RCSWITCH_ASSERT to the system function
 * assert.
 */
#define DEBUG_RCSWITCH false

#if DEBUG_RCSWITCH
#include <assert.h>
/**
 * The size of the pulse tracer.
 * Can be changed.
 */
constexpr size_t MAX_PULSE_TRACES = 64;
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
 * Finally when a message has been received, there can be multiple
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
 * A template function declaration providing initial values for
 * particular types. Will be specialized for the types where the
 * initial value is needed. */
template<typename ELEMENT_TYPE> const ELEMENT_TYPE& INITIAL_VALUE();
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
  /* A variable to store the actual size of the array. */
  size_t mSize;

	inline void init() {
#if DEBUG_RCSWITCH // Initialize only if debugging support is enabled
		std::fill(std::begin(mData), std::end(mData), INITIAL_VALUE<ELEMENT_TYPE>());
#endif
	}

	inline void reset() {
		mSize = 0;
		init();
	}

	Array() : mSize(0) {
	}
public:
	inline size_t size() const {return mSize;}
	inline bool canGrow() const {return mSize < CAPACITY;}
};

/**
 * A container that encapsulates a fixed size stack. Elements can be
 * pushed onto the stack as long as the actual size is smaller than
 * the capacity. Otherwise, the pushed element is dropped, and an
 * overflow counter is increased.
 */
template<typename ELEMENT_TYPE, size_t CAPACITY>
class BlockingStack : public Array<ELEMENT_TYPE, CAPACITY> {
	friend class RcSwitch_test;
protected:
	using baseClass = Array<ELEMENT_TYPE, CAPACITY>;
	using element_type = typename baseClass::element_type;
	/**
	 * A flag that will be raised, when an element couldn't be pushed,
	 * because this stack was already full. */
	uint32_t mOverflow;

	/* Set the actual size of this stack to zero. */
	inline void reset() {baseClass::reset(), mOverflow = 0;}

	/* Default constructor */
	inline BlockingStack() : mOverflow(0) {}
public:
	static constexpr size_t capacity = CAPACITY;

	/**
	 * Return a pointer to the memory, that stores the element
	 * beyond the top stack element. Return null, if there is
	 * no beyond top stack element available.
	 */
	inline element_type* beyondTop() {
		if (baseClass::mSize < CAPACITY) {
			return &baseClass::mData[baseClass::mSize];
		}
		return nullptr;
	}

	/**
	 * Make the beyond top stack element to the top one. if the
	 * stack has still space to take new element. Otherwise
	 * set the overflow flag.
	 */
	inline bool selectNext() {
		if (baseClass::mSize < CAPACITY) {
			++baseClass::mSize;
			return true;
		}
		++mOverflow;
		return false;
	}

	/**
	 * Push an element on top of the stack.
	 */
	bool push(const element_type &value) {
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
	 */
	void remove(const size_t index) {
		if(index < baseClass::mSize) {
#if DEBUG_RCSWITCH // Initialize only if debugging support is enabled
			baseClass::mData[index] = INITIAL_VALUE<element_type>();
#endif
			std::move(&baseClass::mData[index+1], &baseClass::mData[baseClass::mSize],
					&baseClass::mData[index]);
			--baseClass::mSize;
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
class OverwritingStack : public Array<ELEMENT_TYPE, CAPACITY> {
	friend class RcSwitch_test;
	/**
	 * The index of the bottom element of the stack. Use mSize_TYPE
	 * also for mBegin.
	 */
	size_t mBegin;
	static size_t inline squashedIndex(const size_t i) {return (i + CAPACITY) % CAPACITY;}
protected:
	using baseClass = Array<ELEMENT_TYPE, CAPACITY>;
	using element_type = typename baseClass::element_type;
	/* Set the actual size of this stack to zero. */
	inline void reset() {baseClass::reset(); mBegin = 0;}
	/* Default constructor */
	inline OverwritingStack() : mBegin(0) {}
public:
	static constexpr size_t capacity = CAPACITY;

	/**
	 * Return a pointer to the memory, that stores the element
	 * beyond the top stack element.
	 */
	inline element_type* beyondTop() {
		const size_t index = squashedIndex(mBegin + baseClass::mSize);
		return &baseClass::mData[index];
	}

	/**
	 * Make the beyond top stack element to the top element. If
	 * the stack size has already reached the capacity, the
	 * bottom element will be dropped.
	 */
	inline void selectNext() {
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
	inline const element_type& at(const size_t index) const {
		RCSWITCH_ASSERT(index < baseClass::mSize);
		return baseClass::mData[squashedIndex(mBegin + index)];
	}

	/** Refer to methon at() */
	inline const element_type& operator[](const size_t index) const {
		return at(index);
	}

	/**
	 * Return a reference to the element at the specified index.
	 * The index is validated by assert() system function.
	 */
	inline element_type& at(const size_t index) {
		RCSWITCH_ASSERT(index < baseClass::mSize);
		return baseClass::mData[squashedIndex(mBegin + index)];
	}

	/**
	 * Refer to methon at()
	 */
	inline element_type& operator[](const size_t index) {
		return at(index);
	}

};

enum class DATA_BIT : ssize_t{
	UNKNOWN   =-1,
	LOGICAL_0 = 0,
	LOGICAL_1 = 1,
};

#if DEBUG_RCSWITCH
/** Specialize INITIAL_VALUE for DATA_BIT */
template<> inline const DATA_BIT& INITIAL_VALUE<DATA_BIT>() {
	static const DATA_BIT value = DATA_BIT::UNKNOWN;
	return value;
}
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
template<> inline const Pulse& INITIAL_VALUE<Pulse>() {
	static const Pulse value = Pulse{
		0, PULSE_LEVEL::UNKNOWN
	};
	return value;
}
#endif


enum PROTOCOL_GROUP_ID : ssize_t {
	/* Don't change assigned values, because enumerations
	 * are also used as an index into an array.
	 */
	UNKNOWN_PROTOCOL = -1,
	NORMAL_LEVEL_PROTOCOLS  = 0,
	INVERSE_LEVEL_PROTOCOLS = 1,
};

/** A protocol candidate is identified by an index. */
typedef size_t PROTOCOL_CANDIDATE;

#if DEBUG_RCSWITCH
/** Specialize INITIAL_VALUE for PROTOCOL_CANDIDATE */
template<> inline const PROTOCOL_CANDIDATE& INITIAL_VALUE<PROTOCOL_CANDIDATE>() {
	static const PROTOCOL_CANDIDATE value = std::numeric_limits<size_t>::max();
	return value;
}
#endif


/**
 * This container that stores the all the protocols that match
 * the synchronization pulses during the synchronization phase.
 */
class ProtocolCandidates : public BlockingStack<PROTOCOL_CANDIDATE, MAX_PROTOCOL_CANDIDATES> {
	using baseClass = BlockingStack<PROTOCOL_CANDIDATE, MAX_PROTOCOL_CANDIDATES>;
	PROTOCOL_GROUP_ID mProtocolGroupId;

public:
	inline ProtocolCandidates() : mProtocolGroupId(UNKNOWN_PROTOCOL) {
		/** Initialize protocol candidates elements. */
		Array::init();
	}

	/** Remove all protocol candidates from this container */
	inline void reset() {
		baseClass::reset();
		mProtocolGroupId = UNKNOWN_PROTOCOL;
	}

	/**
	 * Push another protocol candidate onto the stack.
	 */
	// Make the base class push() method public.
	using baseClass::push;

	void setProtocolGroup(const PROTOCOL_GROUP_ID protocolGroup) {mProtocolGroupId=protocolGroup;}
	PROTOCOL_GROUP_ID getProtocolGroup()const {return mProtocolGroupId;}
	size_t getProtcolNumber(const size_t protocolCandidateIndex) const;
};


#if DEBUG_RCSWITCH
/**
 * This container stores received pulses for debugging purpose. The
 * purpose is to view the pulses in a debugging session. */
class PulseTracer : public OverwritingStack<Pulse, MAX_PULSE_TRACES> {
	using baseClass = OverwritingStack<Pulse, MAX_PULSE_TRACES>;
public:

	PulseTracer() {
		/* Initialize pulse tracer elements. */
		Array::init();
	}

	/**
	 * Remove all pulses from this pulse tracer container.
	 */
	// Make the base class reset() method public.
	using baseClass::reset;
};
#endif

/**
 * This container stores the received data bits of just one single
 * message packet sent by the transmitter.
 * Note that the transmitter may send the same message packet
 * multiple times.
 * If the transmitter sends more data bits than MAX_MSG_PACKET_BITS,
 * the overflow counter of this container will be incremented.
 * The MAX_MSG_PACKET_BITS constant can be increased and the code
 * can be re-compiled to avoid overflows.
 */
class MessagePacket : public BlockingStack<DATA_BIT, MAX_MSG_PACKET_BITS> {
	using baseClass = BlockingStack<DATA_BIT, MAX_MSG_PACKET_BITS>;

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

#if DEBUG_RCSWITCH
template<> inline const MessagePacket& INITIAL_VALUE<MessagePacket>() {
	static const MessagePacket value;
	return value;
}
#endif

/**
 * The receiver is a buffer that holds the last 2 received pulses.
 * It analyzes these last pulses, whenever a new pulse arrives by
 * a new interrupt. When detecting synchronization pulses the
 * receiver's state changes to DATA_STATE and  converts pulses into received
 * data bits that will be added to the current selected message
 * packet buffer. In case of receiving unexpected pulsed, the
 * receiver goes back to synch state. When a complete message
 * package has been received, and the message packet buffer is
 * still not full, the next message package buffer is
 * selected to receive the next subsequent message packet. Otherwise
 * the message reception is completed and the state becomes
 * AVAILABLE until the reset function is called.
 */
class Receiver : public OverwritingStack<Pulse, DATA_PULSES_PER_BIT> {
	/** =========================================================================== */
	/** == Privately used types, enumerations, variables and methods start here: == */
	using baseClass = OverwritingStack<Pulse, DATA_PULSES_PER_BIT>;
	friend class RcSwitch_test;

	/** API class becomes friend. */
	template<int IOPIN> friend class ::RcSwitchReceiver;

#if DEBUG_RCSWITCH
	/**
	 * For each received message package, the first few received pulses
	 * are stored here for debugging purpose.
	 */
	PulseTracer mPulseTracer;
	/** Store a new pulse in the trace buffer of this message packet. */
	inline void tracePulse(uint32_t microSecDuration, const int pinLevel) {
		Pulse * const currentPulse = mPulseTracer.beyondTop();
		*currentPulse = {microSecDuration, (pinLevel ? PULSE_LEVEL::LO : PULSE_LEVEL::HI)};
		mPulseTracer.selectNext();
	}
#endif

	MessagePacket mReceivedMessagePacket;
	volatile bool mMessageAvailable;
	ProtocolCandidates mProtocolCandidates;
	size_t mReceivedDataModePulseCount;
	uint32_t mMicrosecLastInterruptTime;

	enum STATE {AVAILABLE_STATE, SYNC_STATE, DATA_STATE};
	enum STATE state() const;

	void collectProtocolCandidates(const Pulse&  pulse_0, const Pulse&  pulse_1);
	void push(uint32_t microSecDuration, const int pinLevel);
	PULSE_TYPE analyzePulsePair(const Pulse& firstPulse, const Pulse& secondPulse);
	void retry();

	/** ========================================================================== */
	/** ========= Methods used by API class RcSwitchReceiver start here: ========= */

	/**
	 * Constructor.
	 */
	Receiver()
			: mReceivedDataModePulseCount(0), mMicrosecLastInterruptTime(0), mMessageAvailable(false) {
			/* Initialize pulse elements. */
			Array::init();
	}

	/**
	 * Evaluate a new pulse that has been received. Will only
	 * be called from within interrupt context.
	 */
	void handleInterrupt(const int pinLevel, const uint32_t microSecInterruptTime);

	/**
	 * Remove protocol candidates
	 * for the mProtocolCandidates buffer.
	 * Remove the all data pulses from this container.
	 * Reset the available flag.
	 *
	 * Will be called from inside and outside of the
	 * interrupt handler context.
	 */
	void reset();

	/**
	 * Refer to corresponding API class RcSwitchReceiver;
	 */
	inline bool available() const {return state() == AVAILABLE_STATE;}

	/**
	 * Refer to corresponding API class RcSwitchReceiver;
	 */
	uint32_t receivedValue() const;

	/**
	 * Refer to corresponding API class RcSwitchReceiver;
	 */
	bool receivedBitsCount() const;

	/**
	 * Refer to corresponding API class RcSwitchReceiver;
	 */
	inline size_t receivedProtocolCount() const
		{return mProtocolCandidates.size();}

	/**
	 * Refer to corresponding API class RcSwitchReceiver;
	 */
	int receivedProtocol(const size_t index) const;

};

} /* namespace RcSwitch */

#endif /* RCSWTICH_INTERNAL_HPP_ */
