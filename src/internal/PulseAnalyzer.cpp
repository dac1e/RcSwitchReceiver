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

#include "PulseAnalyzer.hpp"

namespace RcSwitch {

bool PulseAnalyzer::isOfCategorie(const PulseCategory &categorie,
		const Pulse &pulse) const {
	// size_t is 16 bit on avr. So static cast to uint32_t avoids temporary overflow when multiplying with 100
	if (static_cast<uint32_t>(pulse.mMicroSecDuration)
			< ((static_cast<uint32_t>(categorie.microSecDuration)
					* (100 - mPercentTolerance)) / 100)) {
		return false;
	}
	if (static_cast<uint32_t>(pulse.mMicroSecDuration)
			>= ((static_cast<uint32_t>(categorie.microSecDuration)
					* (100 + mPercentTolerance)) / 100)) {
		return false;
	}
	return true;
}

size_t PulseAnalyzer::findCategory(const Pulse &pulse) const {
	for (size_t i = 0; i < size(); i++) {
		if (isOfCategorie(at(i), pulse)) {
			return i;
		}
	}
	return size();
}

PulseAnalyzer::PulseAnalyzer(size_t percentTolerance) :
		mPercentTolerance(percentTolerance) {
}

bool PulseAnalyzer::addPulse(const Pulse &pulse) {
	bool result = true;
	const size_t i = findCategory(pulse);
	if (i >= size()) {
		result = push(
			{
				 pulse.mMicroSecDuration
				,pulse.mPulseLevel == PULSE_LEVEL::LO ? 1 : 0
				,pulse.mPulseLevel == PULSE_LEVEL::HI ? 1 : 0
			}
		);
	} else {
		at(i).addPulse(pulse);
	}
	return result;
}

} /* namespace RcSwitch */
