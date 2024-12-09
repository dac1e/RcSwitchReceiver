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

#include "Pulse.hpp"
#include <Arduino.h>

#if defined(ESP32) || defined(ESP8266)
#include "Pulse.inc"
#endif

namespace RcSwitch {

bool Pulse::isDurationInRange(unsigned int value, unsigned percentTolerance) const {
	// duration_t may be 16 bit. So static cast to uint32_t avoids
	// temporary overflow when multiplying with 100
	if (static_cast<uint32_t>(mUsecDuration)
			< ((static_cast<uint32_t>(value) * (100 - percentTolerance)) / 100)) {
		return false;
	}
	// duration_t may be 16 bit. So static cast to uint32_t avoids
	// temporary overflow when multiplying with 100
	if (static_cast<uint32_t>(mUsecDuration)
			>= ((static_cast<uint32_t>(value) * (100 + percentTolerance)) / 100)) {
		return false;
	}
	return true;
}

PulseCategory::PulseCategory(const Pulse &pulse) :
		mPulse { pulse.getDuration(), pulse.getLevel() }, usecMinDuration(
				pulse.getDuration()), usecMaxDuration(pulse.getDuration()), pulseCount(
				1) {
}

PulseCategory::PulseCategory() :
		mPulse { 0, PULSE_LEVEL::UNKNOWN }, usecMinDuration(0), usecMaxDuration(
				0), pulseCount(0) {
}

bool PulseCategory::addPulse(const Pulse &pulse) {
	bool result = true;
	// Refresh average for the pulse duration and store it.
	mPulse.setDuration(((static_cast<uint32_t>(getWeightedAverage()) * pulseCount)
		+ pulse.getDuration()) / (pulseCount + 1));

	if (pulse.getDuration() < usecMinDuration) {
		usecMinDuration = pulse.getDuration();
	}

	if (pulse.getDuration() > usecMaxDuration) {
		usecMaxDuration = pulse.getDuration();
	}

	if (getPulseLevel() == PULSE_LEVEL::UNKNOWN) {
		mPulse.setLevel(pulse.getLevel());
	} else {
		result = (getPulseLevel() == pulse.getLevel());
	}
	++pulseCount;
	return result;
}

void PulseCategory::merge(PulseCategory &result, const PulseCategory &other) const {
	result.mPulse.setLevel(
			getPulseLevel() == other.getPulseLevel() ?
					getPulseLevel() : PULSE_LEVEL::LO_or_HI);
	result.pulseCount = pulseCount + other.pulseCount;
	result.mPulse.setDuration(
			((pulseCount * getWeightedAverage())
					+ (other.pulseCount * other.getWeightedAverage()))
					/ result.pulseCount);
	result.usecMinDuration =
			usecMinDuration < other.usecMinDuration ?
					usecMinDuration : other.usecMinDuration;
	result.usecMaxDuration =
			usecMaxDuration > other.usecMaxDuration ?
					usecMaxDuration : other.usecMaxDuration;
}

void PulseCategory::invalidate() {
	pulseCount = 0;
	mPulse.setLevel(PULSE_LEVEL::UNKNOWN);
	mPulse.setDuration(0);
	usecMinDuration = INT_TRAITS<typeof (usecMinDuration)>::MAX;
	usecMaxDuration = 0;
}

template<> void PulseCategory::dump(typeof(Serial)& serial, const char* separator) const {
	serial.print("\t");
	printNumWithSeparator(serial, pulseCount, 3, separator);
	printStringWithSeparator(serial, "recordings of", separator);
	printStringWithSeparator(serial, pulseLevelToString(getPulseLevel()), separator);

	serial.print("[");
	serial.print(separator);

	printUsecWithSeparator(serial, usecMinDuration, 5, separator);

	serial.print("..");
	serial.print(separator);

	printUsecWithSeparator(serial, usecMaxDuration, 5, separator);

	serial.print("]");
	serial.print(separator);

	printUsecWithSeparator(serial, getMinMaxAverage(), 5, separator);

	printStringWithSeparator(serial, "+-", separator);
	printPercentWithSeparator(serial, getPercentMinMaxDeviation(), 2, separator);

	serial.println();
}

} /* namespace RcSwitch */
