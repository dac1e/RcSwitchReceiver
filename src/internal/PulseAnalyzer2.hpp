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

static constexpr size_t SYNCH_PULSE_CATEGORIY_COUNT = 2;

// C++ STL not available for avr. So we can not use <algorithm>
int comparePulseCategory(const void* left, const void* right);

template<size_t PULSE_CATEGORIY_COUNT>
class PulseCategoryCollection : public StackBuffer<PulseCategory, PULSE_CATEGORIY_COUNT> {
	using baseClass = StackBuffer<PulseCategory, PULSE_CATEGORIY_COUNT>;
	using baseClass::capacity;
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
	using baseClass::size;
	using baseClass::at;

	void sortByDuration() {
		qsort(&at(0), size(), sizeof(PulseCategory), comparePulseCategory);
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
				{
					 pulse.mPulseLevel
					,pulse.mMicroSecDuration
					,pulse.mMicroSecDuration
					,pulse.mMicroSecDuration
					,1
				}
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

	void build(const RingBufferReadAccess<Pulse>& input, unsigned percentTolerance
			,synchPulseCategories_t& synchPulseCategories, size_t microsecSynchB) {
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
	}

	template <typename T>
	void dump(T& stream, const char* separator) const {
		stream.println("Identified pulse categories:");

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
	static constexpr size_t MAX_PULSE_CATEGORIES = 6;
	static constexpr size_t DATA_PULSE_CATEGORIY_COUNT =
			MAX_PULSE_CATEGORIES - SYNCH_PULSE_CATEGORIY_COUNT;

	const unsigned mPercentTolerance;
	const RingBufferReadAccess<Pulse> mInput;

	PulseCategoryCollection<MAX_PULSE_CATEGORIES> mAllPulseCategories;
	PulseCategoryCollection<SYNCH_PULSE_CATEGORIY_COUNT> mSynchPulseCategories;
	PulseCategoryCollection<DATA_PULSE_CATEGORIY_COUNT> mDataPulseCategories;

	bool buildSynchAndDataCategories();
	void buildAllCategories();
public:
	PulseAnalyzer2(const RingBufferReadAccess<Pulse>& input, unsigned percentTolerance = 20);

	void analyze() {
		buildAllCategories();
		if(not mAllPulseCategories.overflowCount()) {
			buildSynchAndDataCategories();
		}
	}

	template <typename T>
	void dump(T& stream, const char* separator) {
		mAllPulseCategories.dump(stream, separator);
		if(mSynchPulseCategories.size()) {
			mSynchPulseCategories.dump(stream, separator);
		}
		if(mDataPulseCategories.size()) {
			mDataPulseCategories.dump(stream, separator);
		}
	}
};

} /* namespace RcSwitch */

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER2_HPP_ */
