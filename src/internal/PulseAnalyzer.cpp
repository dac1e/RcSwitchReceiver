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

// C++ STL not available for avr. So we can not use <algorithm>

#include <stdlib.h>
#include <assert.h>
#include "ISR_ATTR.hpp"
#include "Container.hpp"
#include "PulseAnalyzer.hpp"

namespace {

inline bool operator < (const RcSwitch::PulseCategory& this_, const RcSwitch::PulseCategory& other) {
	bool result = false;
	// Order criteria is duration and then the level.
	if(this_.microSecDuration < other.microSecDuration) {
		result = true;
	} else if(this_.microSecDuration == other.microSecDuration) {
		result = this_.pulseLevel < other.pulseLevel;
	}
	return result;
}

int comparePulseCategory(const void* pa, const void* pb) {
	const RcSwitch::PulseCategory* a = static_cast<const RcSwitch::PulseCategory*>(pa);
	const RcSwitch::PulseCategory* b = static_cast<const RcSwitch::PulseCategory*>(pb);

	// This is for descending order
	if(*a < *b) return 1;
	if(*b < *a) return -1;
	return 0;
}


} // anonymous namespace

namespace RcSwitch {

const PulseCategory& PulseAnalyzer::getLowPulseFromPair(size_t begin, bool& bOk) const {
	if(at(begin).pulseLevel == PULSE_LEVEL::LO) {
		return at(begin);
	}
    bOk &= (at(begin+1).pulseLevel == PULSE_LEVEL::LO);
	return at(begin+1);
}

const PulseCategory& PulseAnalyzer::getHighPulseFromPair(size_t begin, bool& bOk) const {
	if(at(begin).pulseLevel == PULSE_LEVEL::HI) {
		return at(begin);
	}
	bOk &= (at(begin+1).pulseLevel == PULSE_LEVEL::HI);
	return at(begin+1);
}

void PulseAnalyzer::sortDescendingByDuration() {
	qsort(&at(0), size(), sizeof(PulseCategory), comparePulseCategory);
}

void PulseAnalyzer::analyze() {
	if(buildCategoriesFromPulses()) {
		// Guess protocol.
		if(mCategoryOfSynchPulseA.isValid() && mCategoryOfSynchPulseB.isValid()) {
			bool bOk = true;
			mInverse = mCategoryOfSynchPulseA.pulseLevel == PULSE_LEVEL::LO;
			if(not mInverse) {
				// longer pulse
				mDataPulses[0][B] = &getLowPulseFromPair(0, bOk);
				mDataPulses[1][A] = &getHighPulseFromPair(0, bOk);

				// shorter pulse
				mDataPulses[0][A] = &getHighPulseFromPair(2, bOk);
				mDataPulses[1][B] = &getLowPulseFromPair(2, bOk);
			} else {
				// longer pulse
				mDataPulses[0][B] = &getHighPulseFromPair(0, bOk);
				mDataPulses[1][A] = &getLowPulseFromPair(0, bOk);

				// shorter pulse
				mDataPulses[0][A] = &getLowPulseFromPair(2, bOk);
				mDataPulses[1][B] = &getHighPulseFromPair(2, bOk);
			}

			if(!bOk) {
				invalidateProtocolGuess();
			}
		}
	}
}

bool PulseAnalyzer::isOfCategorie(const PulseCategory &categorie,
		const Pulse &pulse) const {

	if(pulse.mPulseLevel != categorie.pulseLevel) {
		return false;
	}

	return pulse.isDurationInRange(categorie.microSecDuration, mPercentTolerance);;
}

size_t PulseAnalyzer::findCategory(const Pulse &pulse) const {
	for (size_t i = 0; i < size(); i++) {
		if (isOfCategorie(at(i), pulse)) {
			return i;
		}
	}
	return size();
}

PulseAnalyzer::PulseAnalyzer(const RingBufferReadAccess<Pulse>& pulses, unsigned percentTolerance)
	:mPulses(pulses)
	,mPercentTolerance(percentTolerance)
	,mCategoryOfSynchPulseA({PULSE_LEVEL::UNKNOWN, 0, static_cast<size_t>(-1), 0 ,0})
	,mCategoryOfSynchPulseB({PULSE_LEVEL::UNKNOWN, 0, static_cast<size_t>(-1), 0 ,0})
	,mDataPulses({{nullptr,nullptr},{nullptr,nullptr}})
	,mInverse(false)
{
}

bool PulseAnalyzer::addPulse(size_t i) {
	bool result = true;
	const Pulse &pulse = mPulses.at(i);
	if(not mCategoryOfSynchPulseB.isValid()) {
		// This is the first run, where we don't have the synch pulse B yet
		const size_t i = findCategory(pulse);
		if (i >= size()) {
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
			result = at(i).addPulse(pulse);
		}
	} else {
		// Add, if it is not a synch pulse B.
		if(not pulse.isDurationInRange(mCategoryOfSynchPulseB.microSecDuration, mPercentTolerance)) {
			// It is not a synch B pulse. So just insert it.
			const size_t i = findCategory(pulse);
			if (i >= size()) {
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
				result = at(i).addPulse(pulse);
			}
		} else {
			// It is a synch pulse B, so we grab the synch pulse A
			if(i > 0) {
				const Pulse &synchPulseA = mPulses.at(i-1);
				result = mCategoryOfSynchPulseA.addPulse(synchPulseA);
			}
		}
	}
	return result;
}

void PulseAnalyzer::invalidateProtocolGuess() {
	mCategoryOfSynchPulseA.invalidate();
	mCategoryOfSynchPulseB.invalidate();
	mDataPulses[0][A] = nullptr;
	mDataPulses[0][B] = nullptr;
	mDataPulses[1][A] = nullptr;
	mDataPulses[1][B] = nullptr;
	mInverse = false;
}

void PulseAnalyzer::reset() {
	baseClass::reset();
	invalidateProtocolGuess();
}

bool PulseAnalyzer::buildCategoriesFromPulses() {
	bool result = true;
	const size_t n = mPulses.size();
	if(n) {
		// The category with the longest duration will provide the second
		// synch pulse (synch. pulse B).
		if(not mCategoryOfSynchPulseB.isValid()) {
			// This is the first run, which is just for finding the
			// synch pulse B category.
			size_t i = 0;
			for(; i < n; i++) {
				result &= addPulse(i);
			}
			sortDescendingByDuration();

			if(result) {
				// The first one is the one with the longest duration.
				mCategoryOfSynchPulseB = at(0);

				// Clear the categories for a second run, but now with the
				// second synch pulse duration already known.
				baseClass::reset();

				// Do second run.
				result = buildCategoriesFromPulses();
			}
		} else {
			// This is the second run, which builds only data pulse
			// categories and collects the synch pulse A category.
			size_t i = 0;
			for(; i < n; i++) {
				result &= addPulse(i);
			}
			sortDescendingByDuration();
		}
	}
	return result;
}

} /* namespace RcSwitch */
