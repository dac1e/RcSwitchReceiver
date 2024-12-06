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

#ifndef RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER2_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER2_HPP_

#include <stddef.h>
#include <stdlib.h>

#include "ISR_ATTR.hpp"
#include "Pulse.hpp"
#include "Container.hpp"

namespace RcSwitch {

/**
 * The 6 potential different categories are:
 *
 * synch A
 * synch B
 *
 * synch data0 A
 * synch data0 B
 *
 * synch data0 A
 * synch data0 B
 */
static constexpr size_t SYNCH_PULSE_CATEGORIY_COUNT = 2;
static constexpr size_t DATA_PULSE_CATEGORIY_COUNT  = 4;
static constexpr size_t ALL_PULSE_CATEGORY_COUNT =
	SYNCH_PULSE_CATEGORIY_COUNT + DATA_PULSE_CATEGORIY_COUNT;

/**
 * The synch pulse B must be longer than synch pulse A to be recognized as
 * a valid synch pulse pair.
 */
static constexpr uint32_t SYNCH_PULSES_MIN_RATIO = 8;
/**
 * The data pulse B must be longer than data pulse A to be recognized as
 * a valid data pulse pair.
 */
static constexpr double DATA_PULSES_MIN_RATIO = 1.5;

// C++ STL not available for avr. So we can not use <algorithm>
int comparePulseCategoryByDuration(const void* left, const void* right);
int comparePulseCategoryByLevel(const void* left, const void* right);

struct DataPulses {

	static constexpr uint32_t PERCENT_DATA_PULSES_MIN_RATIO = 100 * DATA_PULSES_MIN_RATIO;

	const PulseCategory* d0A;
	const PulseCategory* d0B;
	const PulseCategory* d1A;
	const PulseCategory* d1B;

	bool bIsInverseLevel;

	DataPulses()
		: d0A(nullptr), d0B(nullptr), d1A(nullptr), d1B(nullptr), bIsInverseLevel(false)
	{
	}

	void reset() {
		d0A = d0B = d1A = d1B = nullptr;
		bIsInverseLevel = false;
	}

	size_t getDurationD0A() {
		return d0A->microSecDuration; // short pulse
	}
	size_t getDurationD0B() {
		return d0B->microSecDuration; // long pulse
	}
	size_t getDurationD1A() {
		return d1A->microSecDuration; // long pulse
	}
	size_t getDurationD1B() {
		return d1B->microSecDuration; // short pulse
	}

	bool checkRatio() {
		if(100 * static_cast<uint32_t>(d0B->microSecDuration) // long pulse
				< PERCENT_DATA_PULSES_MIN_RATIO * static_cast<uint32_t>(d0A->microSecDuration)) {
			return false;
		}
		if(100 * static_cast<uint32_t>(d1A->microSecDuration) // long pulse
				< PERCENT_DATA_PULSES_MIN_RATIO * static_cast<uint32_t>(d1B->microSecDuration)) {
			return false;
		}
		return true;
	}

	bool isValid() {
		if(d0A && d0B && d1A && d1B) {
			return checkRatio();
		}
		return false;
	}
};

template<size_t PULSE_CATEGORIY_COUNT>
class PulseCategoryCollection : public StackBuffer<PulseCategory, PULSE_CATEGORIY_COUNT> {
	using baseClass = StackBuffer<PulseCategory, PULSE_CATEGORIY_COUNT>;
	using baseClass::push;

	using synchPulseCategories_t = PulseCategoryCollection<SYNCH_PULSE_CATEGORIY_COUNT>;;

	static bool pulseFitsInCategory(const PulseCategory& category, const Pulse &pulse
			, unsigned percentTolerance) {
		if(pulse.mPulseLevel != category.pulseLevel) {
			return false;
		}
		return pulse.isDurationInRange(category.microSecDuration
				, percentTolerance);
	}

public:
	using baseClass::overflowCount;
	using baseClass::capacity;
	using baseClass::size;
	using baseClass::at;
	using baseClass::reset;

	inline void sortByDuration(PulseCategory& first, const size_t size) {
		qsort(&first, size, sizeof(PulseCategory), comparePulseCategoryByDuration);
	}

	inline void sortByLevel(PulseCategory& first, const size_t size) {
		qsort(&first, size, sizeof(PulseCategory), comparePulseCategoryByLevel);
	}

	inline void sortByDuration() {
		sortByDuration(at(0), size());
	}

	inline void sortPairsByDuration() {
		// Sort data pulse pairs by level
		for(size_t i = 0; (i+1) < size(); i += 2) {
			sortByDuration(at(i), 2);
		}
	}

	size_t findCategoryForPulse(const Pulse &pulse, unsigned percentTolerance) const {
		size_t i = 0;
		for (; i < size(); i++) {
			if (pulseFitsInCategory(at(i), pulse, percentTolerance)) {
				return i;
			}
		}
		return i;
	}

	void putPulseInCategory(size_t categoryIndex, const Pulse &pulse) {
		if (categoryIndex >= size()) {
			push(
				{pulse.mPulseLevel,pulse.mMicroSecDuration,pulse.mMicroSecDuration,pulse.mMicroSecDuration,1}
			);
		} else {
			at(categoryIndex).addPulse(pulse);
		}
	}

	void build(const RingBufferReadAccess<Pulse>& input, unsigned percentTolerance) {
		size_t i = 0;
		for(; i < input.size(); i++) {
			const Pulse &pulse = input.at(i);
			const size_t ci = findCategoryForPulse(pulse, percentTolerance);
			putPulseInCategory(ci, pulse);
		}
		sortByDuration();
	}

	void build(DataPulses& dataPulses, const RingBufferReadAccess<Pulse>& input, unsigned percentTolerance
			,synchPulseCategories_t& synchPulseCategories, size_t microsecSynchB) {

		assert(capacity == DATA_PULSE_CATEGORIY_COUNT); // // This must be the data pulse category collection

		size_t i = 0;
		for(; i < input.size(); i++) {
			if((i+1) < input.size()) {
				// It is not the last pulse
				const Pulse &nextPulse = input.at(i+1);
				if(nextPulse.isDurationInRange(microsecSynchB, percentTolerance)) {
					{
						// It is the synch A pulse
						const Pulse &pulse = input.at(i);
						const size_t ci = synchPulseCategories.findCategoryForPulse(pulse, percentTolerance);
						synchPulseCategories.putPulseInCategory(ci, pulse);
					}
				} else {
					const Pulse &pulse = input.at(i);
					if(pulse.isDurationInRange(microsecSynchB, percentTolerance)) {
						// It is a synch B pulse, place it in the synch pulse collection
						const size_t ci = synchPulseCategories.findCategoryForPulse(pulse, percentTolerance);
						synchPulseCategories.putPulseInCategory(ci, pulse);
					} else {
						// It is a data pulse, place it in this pulse collection
						const size_t ci = findCategoryForPulse(pulse, percentTolerance);
						putPulseInCategory(ci, pulse);
					}
				}
			} else {
				// It is the last pulse
				const Pulse &pulse = input.at(i);
				if(pulse.isDurationInRange(microsecSynchB, percentTolerance)) {
					// It is a synch B pulse, place it in the synch pulse collection
					const size_t ci = synchPulseCategories.findCategoryForPulse(pulse, percentTolerance);
					synchPulseCategories.putPulseInCategory(ci, pulse);
				} // else it might be a data pulse or a synch A pulse. This is unknown, hence just drop it.
			}
		}
		synchPulseCategories.sortByDuration();
		sortByDuration();

		if(synchPulseCategories.size() == synchPulseCategories.capacity) {
			// Both synch pulses discovered.
			if(synchPulseCategories.isValidSynchPulsePair()) {
				if(size() == capacity) {
					// There are sufficient data pulse pairs.
					sortByLevel(at(0), size());
					sortPairsByDuration();

					dataPulses.bIsInverseLevel = synchPulseCategories.at(0).pulseLevel == PULSE_LEVEL::LO;
					if(dataPulses.bIsInverseLevel) {
						dataPulses.d0A = &at(0); // short time low
						dataPulses.d0B = &at(3); // long time high
						dataPulses.d1A = &at(1); // long time low
						dataPulses.d1B = &at(2); // short time high
					} else {
						dataPulses.d0A = &at(2); // short time high
						dataPulses.d0B = &at(1); // long time low
						dataPulses.d1A = &at(3); // long time high
						dataPulses.d1B = &at(0); // short time low
					}
				}
			}
		}
	}

	bool isValidSynchPulsePair() {
		assert(capacity == SYNCH_PULSE_CATEGORIY_COUNT); // This must be the synch pulse category collection
		if(size() == SYNCH_PULSE_CATEGORIY_COUNT) {
			// Note that synch pulses are sorted in ascending order of duration.
			const PulseCategory& shorterPulse = at(0);
			const PulseCategory& longerPulse = at(1);
			if(static_cast<uint32_t>(longerPulse.microSecDuration) > SYNCH_PULSES_MIN_RATIO * shorterPulse.microSecDuration) {
				return true;
			}
		}
		return false;
	}

	static constexpr uint32_t DATA_PULSES_MIN_RATIO_PERCENT = 100 * DATA_PULSES_MIN_RATIO;

	template <typename T>
	void dump(T& stream, const char* separator) const {
		for(size_t i = 0; i < size(); i++) {
			const PulseCategory& pulseCategory = at(i);
			pulseCategory.dump(stream, separator);
		}

		if(overflowCount()) {
			stream.print("Warning! ");
			stream.print(overflowCount());
			stream.print(" more categories could not be recorded.");
			stream.print(" Category recording capacity is ");
			stream.print(capacity);
			stream.println('.');
		}
	}

};

class PulseAnalyzer2 {
	const RingBufferReadAccess<Pulse> mInput;
	const unsigned mPercentTolerance;

	PulseCategoryCollection<ALL_PULSE_CATEGORY_COUNT> mAllPulseCategories;
	PulseCategoryCollection<SYNCH_PULSE_CATEGORIY_COUNT> mSynchPulseCategories;
	PulseCategoryCollection<DATA_PULSE_CATEGORIY_COUNT> mDataPulseCategories;

	DataPulses mDataPulses;

	void buildSynchAndDataCategories();
	void buildAllCategories();

public:
	PulseAnalyzer2(const RingBufferReadAccess<Pulse>& input, unsigned percentTolerance = 20);

	void analyze() {
		buildAllCategories();
		if(mAllPulseCategories.size()) {
			if(not mAllPulseCategories.overflowCount()) {
				buildSynchAndDataCategories();
			}
		}
	}

	template <typename T>
	void dump(T& stream, const char* separator) {
		stream.println("Identified pulse categories:");
		mAllPulseCategories.dump(stream, separator);

		if(mSynchPulseCategories.size()) {
			stream.println("Identified synch pulse categories:");
			mSynchPulseCategories.dump(stream, separator);
		}

		if(mDataPulseCategories.size()) {
			stream.println("Identified data pulse categories:");
			mDataPulseCategories.dump(stream, separator);
		}

//		 *  //                   #, clk,  %, syA,  syB,  d0A,d0B,  d1A,d1B, inverseLevel
//		 *  	makeTimingSpec<  1, 350, 20,   1,   31,    1,  3,    3,  1, false>, // (PT2262)

		if(mDataPulses.isValid()) {
			stream.print("makeTimingSpec< #, ");
			stream.print(mPercentTolerance);
			stream.print(", ");
			stream.print(mDataPulses.d0A->microSecDuration);
			stream.print(", ");
			stream.print(mDataPulses.d0B->microSecDuration);
			stream.print(", ");
			stream.print(mDataPulses.d1A->microSecDuration);
			stream.print(", ");
			stream.print(mDataPulses.d1B->microSecDuration);
			stream.print(", ");
			stream.print(mDataPulses.bIsInverseLevel);
			stream.println(">");
		}
	}
};

} /* namespace RcSwitch */

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER2_HPP_ */
