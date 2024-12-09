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

#ifndef RCSWITCH_RECEIVER_INTERNAL_CONTAINER_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_CONTAINER_HPP_

#define DEBUG_RCSWITCH_CONTAINER false

#include <stddef.h>
#include <stdint.h>
#include "ISR_ATTR.hpp"

/**
 * Setting DEBUG_RCSWITCH_CONTAINER to true will map macro
 * RCSWITCH_CONTAINER_ASSERT to the system function assert.
 */
#if DEBUG_RCSWITCH_CONTAINER
#include <assert.h>
#define RCSWITCH_CONTAINER_ASSERT assert
#else
#define RCSWITCH_CONTAINER_ASSERT(expr)
#endif

namespace RcSwitch {

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

	TEXT_ISR_ATTR_1 inline void reset() {
		mSize = 0;
	}

	/** Default constructor */
	Array() : mSize(0) {
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
	 * A counter that will be incremented
	 * when an element couldn't be pushed,
	 * because this stack was already full. */
	uint32_t mOverflow;

	/* Set the actual size of this stack to zero and clear
	 * the overflow counter.
	 */
	TEXT_ISR_ATTR_2 inline void reset() {baseClass::reset(), mOverflow = 0;}

	/* Default constructor */
	inline StackBuffer() : mOverflow(0) {}
public:
	/**
	 * Make the capacity template argument available as
	 * const expression. */
	static constexpr size_t capacity = CAPACITY;

	/**
	 * Check whether this stack buffer is full, without any overflow
	 */
	inline bool isAtTheEdge() const {
		return not mOverflow && (baseClass::size() == capacity);
	}

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
	 * The index is validated by the assert() system function.
	 */
	inline const element_type& at(const size_t index) const {
		RCSWITCH_CONTAINER_ASSERT(index < baseClass::mSize);
		return baseClass::mData[index];
	}

	/**
	 * Return a const reference to the element at the specified index.
	 * The index is validated by the assert() system function.
	 */
	inline element_type& at(const size_t index) {
		RCSWITCH_CONTAINER_ASSERT(index < baseClass::mSize);
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
			size_t i = index+1;
			for(;i < baseClass::mSize; i++) {
				baseClass::mData[i-1] = baseClass::mData[i];
			}
			--baseClass::mSize;
		}
	}

	/* Return the value of the overflow counter. */
	inline size_t overflowCount()const {return mOverflow;}
};

/**
 * Forward declaration of RingBufferReadAccess.
 */
template<typename ELEMENT_TYPE> class RingBufferReadAccess;

/**
 * A container that encapsulates a fixed size ring buffer.
 * Elements can be pushed on the ring buffer. When the
 * size of the stack has reached the capacity, the bottom
 * element will be dropped on cost of a new pushed element.
 */
template<typename ELEMENT_TYPE, size_t CAPACITY>
class RingBuffer : public Array<ELEMENT_TYPE, CAPACITY> {
	friend class RcSwitch_test;
	friend class RingBufferReadAccess<ELEMENT_TYPE>;

	/** The index of the bottom element of the ring buffer. */
	size_t mBegin;

	static TEXT_ISR_ATTR_2 inline size_t squashedIndex(const size_t i) {
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
	/** Make the capacity template argument available as
	 * const expression. */
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
	 * Make the beyond top ring buffer element to the top element.
	 * If the ring buffer size has already reached the capacity,
	 * the bottom element will be dropped.
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
	 * The index is validated by the assert() system function.
	 */
	TEXT_ISR_ATTR_1 inline const element_type& at(const size_t index) const {
		RCSWITCH_CONTAINER_ASSERT(index < baseClass::mSize);
		return baseClass::mData[squashedIndex(mBegin + index)];
	}

	/**
	 * Return a reference to the element at the specified index.
	 * The index is validated by the assert() system function.
	 */
	TEXT_ISR_ATTR_1 inline element_type& at(const size_t index) {
		RCSWITCH_CONTAINER_ASSERT(index < baseClass::mSize);
		return baseClass::mData[squashedIndex(mBegin + index)];
	}
};

/**
 * This class allows indexed read access to a ring buffer of any size.
 */
template<typename ELEMENT_TYPE> class RingBufferReadAccess {
	const ELEMENT_TYPE* mData;
	const size_t mCapacity;
	const size_t mSize;
	const size_t mBegin;

	inline size_t squashedIndex(const size_t i) const {
		return (i + mCapacity) % mCapacity;
	}

public:
	template<size_t CAPACITY>
	RingBufferReadAccess(const RingBuffer<ELEMENT_TYPE, CAPACITY>& ringBuffer)
		: mData(ringBuffer.mData), mCapacity(CAPACITY), mSize(ringBuffer.size()), mBegin(ringBuffer.mBegin) {
	}

	/**
	 * Return a const reference to the element at the specified index.
	 * The index is validated by the assert() system function.
	 */
	inline const ELEMENT_TYPE& at(const size_t index) const {
		RCSWITCH_CONTAINER_ASSERT(index < mSize);
		return mData[squashedIndex(mBegin + index)];
	}

	size_t size() const {return mSize;}
};


} // namespace RcSwitch

#endif /* RCSWITCH_RECEIVER_INTERNAL_CONTAINER_HPP_ */
