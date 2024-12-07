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
#include "Common.hpp"
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

	inline uint32_t getDurationD0A() {
		return d0A->getWeightedAverage(); // short pulse
	}
	inline uint32_t getDurationD0B(uint16_t scaleBase = 1) {
		return d0B->getWeightedAverage(); // long pulse
	}
	inline uint32_t getDurationD1A(uint16_t scaleBase = 1) {
		return d1A->getWeightedAverage(); // long pulse
	}
	inline uint32_t getDurationD1B(uint16_t scaleBase = 1) {
		return d1B->getWeightedAverage(); // short pulse
	}

	inline uint32_t getMinMaxAverageD0A(uint16_t scaleBase = 1) {
		return scale(d0A->getMinMaxAverage(), scaleBase); // short pulse
	}
	inline uint32_t getMinMaxAverageD0B(uint16_t scaleBase = 1) {
		return scale(d0B->getMinMaxAverage(), scaleBase); // long pulse
	}
	inline uint32_t getMinMaxAverageD1A(uint16_t scaleBase = 1) {
		return scale(d1A->getMinMaxAverage(), scaleBase); // long pulse
	}
	inline uint32_t getMinMaxAverageD1B(uint16_t scaleBase = 1) {
		return scale(d1B->getMinMaxAverage(), scaleBase); // short pulse
	}

	bool checkRatio() {
		if(100 * getDurationD0B() // long pulse
				< PERCENT_DATA_PULSES_MIN_RATIO * getDurationD0A()) {
			return false;
		}
		if(100 * getDurationD1A() // long pulse
				< PERCENT_DATA_PULSES_MIN_RATIO * getDurationD1B()) {
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
		if(pulse.mPulseLevel != category.getPulseLevel()) {
			return false;
		}
		return pulse.isDurationInRange(category.getWeightedAverage()
				, percentTolerance);
	}

public:
	using baseClass::overflowCount;
	using baseClass::capacity;
	using baseClass::size;
	using baseClass::at;
	using baseClass::isAtTheEdge;
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
			push(pulse);
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
			,synchPulseCategories_t& synchPulseCategories, size_t usecSynchB) {

		assert(capacity == DATA_PULSE_CATEGORIY_COUNT); // // This must be the data pulse category collection

		size_t i = 0;
		for(; i < input.size(); i++) {
			if((i+1) < input.size()) {
				// It is not the last pulse
				const Pulse &nextPulse = input.at(i+1);
				if(nextPulse.isDurationInRange(usecSynchB, percentTolerance)) {
					{
						// It is the synch A pulse
						const Pulse &pulse = input.at(i);
						const size_t ci = synchPulseCategories.findCategoryForPulse(pulse, percentTolerance);
						synchPulseCategories.putPulseInCategory(ci, pulse);
					}
				} else {
					const Pulse &pulse = input.at(i);
					if(pulse.isDurationInRange(usecSynchB, percentTolerance)) {
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
				if(pulse.isDurationInRange(usecSynchB, percentTolerance)) {
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

					dataPulses.bIsInverseLevel = synchPulseCategories.at(0).getPulseLevel() == PULSE_LEVEL::LO;
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
			if(static_cast<uint32_t>(longerPulse.getWeightedAverage()) > SYNCH_PULSES_MIN_RATIO * shorterPulse.getWeightedAverage()) {
				return true;
			}
		}
		return false;
	}

	uint32_t getDurationSyA(uint16_t scaleBase = 1) {
		assert(capacity == SYNCH_PULSE_CATEGORIY_COUNT); // This must be the synch pulse category collection
		const PulseCategory& pulseCategorySyA = at(0);
		return scale(pulseCategorySyA.getWeightedAverage(), scaleBase);
	}

	uint32_t getDurationSyB(uint16_t scaleBase = 1) {
		assert(capacity == SYNCH_PULSE_CATEGORIY_COUNT); // This must be the synch pulse category collection
		const PulseCategory& pulseCategorySyB = at(1);
		return scale(pulseCategorySyB.getWeightedAverage(), scaleBase);
	}

	static constexpr uint32_t DATA_PULSES_MIN_RATIO_PERCENT = 100 * DATA_PULSES_MIN_RATIO;

	template <typename T>
	void dump(T& stream, const char* separator) const {
		for(size_t i = 0; i < size(); i++) {
			const PulseCategory& pulseCategory = at(i);
			pulseCategory.dump(stream, separator);
		}
	}

};

class PulseAnalyzer {
	const RingBufferReadAccess<Pulse> mInput;
	const unsigned mPercentTolerance;

	PulseCategoryCollection<ALL_PULSE_CATEGORY_COUNT> mAllPulseCategories;
	PulseCategoryCollection<SYNCH_PULSE_CATEGORIY_COUNT> mSynchPulseCategories;
	PulseCategoryCollection<DATA_PULSE_CATEGORIY_COUNT> mDataPulseCategories;

	DataPulses mDataPulses;

	void buildSynchAndDataCategories();
	void buildAllCategories();

public:
	PulseAnalyzer(const RingBufferReadAccess<Pulse>& input, unsigned percentTolerance = 20);

	void dedcuceProtocol() {
		buildAllCategories();
		if(mAllPulseCategories.size()) {
			if(not mAllPulseCategories.overflowCount()) {
				buildSynchAndDataCategories();
			}
		}
	}

	template <typename T>
	void dumpProposedTimings(T& stream, uint16_t clock) {
		if(mSynchPulseCategories.isValidSynchPulsePair()) {
			if(mDataPulses.isValid()) {
				stream.print("makeTimingSpec< #,");
				printUint(stream, clock, 3, ",");
				printUint(stream, mPercentTolerance, 3, ",");
				printUint(stream, mSynchPulseCategories.getDurationSyA(clock), 3, ",");
				printUint(stream, mSynchPulseCategories.getDurationSyB(clock), 4, ",");
				printUint(stream, mDataPulses.getMinMaxAverageD0A(clock), 4, ",");
				printUint(stream, mDataPulses.getMinMaxAverageD0B(clock), 4, ",");
				printUint(stream, mDataPulses.getMinMaxAverageD1A(clock), 4, ",");
				printUint(stream, mDataPulses.getMinMaxAverageD1B(clock), 4, ",");
				stream.print(((mDataPulses.bIsInverseLevel) ? " true" : " false"));
				stream.println(">");
				stream.println("------- Replace the '#' above by a unique identifier -------");
			}
		}
	}

	template <typename T>
	void dump(T& stream, const char* separator) {
		stream.println("Identified COMMON pulse categories:");
		mAllPulseCategories.dump(stream, separator);

		if(mSynchPulseCategories.size()) {
			stream.println("\nIdentified SYNCH pulse categories:");
			mSynchPulseCategories.dump(stream, separator);
		}

#if false
		if(mSynchPulseCategories.overflowCount()) {
			stream.print("Warning! ");
			stream.print(mSynchPulseCategories.overflowCount());
			stream.print(" more categories could not be recorded.");
			stream.print(" Category recording capacity is ");
			stream.print(mSynchPulseCategories.capacity);
			stream.println('.');
		}
#endif

		if(mDataPulseCategories.size()) {
			stream.println("\nIdentified DATA pulse categories:");
			mDataPulseCategories.dump(stream, separator);
		}

#if false
		if(mDataPulseCategories.overflowCount()) {
			stream.print("Warning! ");
			stream.print(mDataPulseCategories.overflowCount());
			stream.print(" more categories could not be recorded.");
			stream.print(" Category recording capacity is ");
			stream.print(mDataPulseCategories.capacity);
			stream.println('.');
		}
#endif

		const bool bOk = mSynchPulseCategories.isAtTheEdge() && mDataPulseCategories.isAtTheEdge() && mDataPulses.isValid();
		if(bOk) {
#if false
			{
				stream.println("All SHORT DATA pulse categories together:");
				PulseCategory shortPulses;
				mDataPulses.d0A->merge(shortPulses, *mDataPulses.d1B);
				shortPulses.dump(stream, separator);
			}

			{
				stream.println("All LONG DATA pulse categories together:");
				PulseCategory longPulses;
				mDataPulses.d1A->merge(longPulses, *mDataPulses.d0B);
				longPulses.dump(stream, separator);
			}
#endif
			static const char* const frame =
						   "**************************************************************";
			stream.println("\n"
						   "Protocol detection succeeded. Protocol proposal:");
			stream.println(frame);
			dumpProposedTimings(stream, 10);
			stream.println(frame);
		} else {
			stream.println("\n"
					       "Protocol detection failed. Please try again. You may\n"
						   "reposition your Remote Control a bit or use a different\n"
						   "RC button. Be sure that you press the RC button at\n"
						   "least for 3 seconds, before you start the pulse trace.");
		}
	}
};

} /* namespace RcSwitch */

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER2_HPP_ */