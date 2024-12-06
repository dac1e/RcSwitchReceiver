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

#include "PulseAnalyzer2.hpp"

namespace RcSwitch {

int comparePulseCategoryByDuration(const void* left, const void* right) {
	const RcSwitch::PulseCategory* a = static_cast<const RcSwitch::PulseCategory*>(left);
	const RcSwitch::PulseCategory* b = static_cast<const RcSwitch::PulseCategory*>(right);

	// This is for ascending order
	if(a->getDuration() < b->getDuration()) return -1;
	if(b->getDuration() < a->getDuration()) return 1;
	return 0;
}

int comparePulseCategoryByLevel(const void* left, const void* right) {
	const RcSwitch::PulseCategory* a = static_cast<const RcSwitch::PulseCategory*>(left);
	const RcSwitch::PulseCategory* b = static_cast<const RcSwitch::PulseCategory*>(right);

	// This is for ascending order
	if(a->getPulseLevel() < b->getPulseLevel()) return -1;
	if(b->getPulseLevel() < a->getPulseLevel()) return 1;
	return 0;
}

PulseAnalyzer2::PulseAnalyzer2(const RingBufferReadAccess<Pulse>& input, unsigned percentTolerance)
	:mInput(input)
	,mPercentTolerance(percentTolerance)
{
}

void PulseAnalyzer2::buildAllCategories() {
	mAllPulseCategories.reset();
	mAllPulseCategories.build(mInput, mPercentTolerance);
}

void PulseAnalyzer2::buildSynchAndDataCategories() {
	mDataPulses.reset();
	mDataPulseCategories.reset();
	mSynchPulseCategories.reset();
	if (mAllPulseCategories.size() >= 0) {
		mDataPulseCategories.build(mDataPulses, mInput, mPercentTolerance, mSynchPulseCategories,
			mAllPulseCategories.at(mAllPulseCategories.size()-1).getDuration());
	}
}

} /* namespace RcSwitch */
