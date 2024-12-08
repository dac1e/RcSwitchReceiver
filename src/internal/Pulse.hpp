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

#ifndef RCSWITCH_RECEIVER_INTERNAL_PULSE_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_PULSE_HPP_

#include <stdint.h>
#include <assert.h>

#include "FormattedPrint.hpp"
#include "TypeTraits.hpp"

namespace RcSwitch {

enum class PULSE_LEVEL {
	UNKNOWN = 0,
	LO,
	HI,
	LO_or_HI,
};

inline const char* pulseLevelToString(const PULSE_LEVEL& pulseLevel) {
	switch(pulseLevel) {
	case PULSE_LEVEL::LO:
		return " LOW";
	case PULSE_LEVEL::HI:
		return "HIGH";
	case PULSE_LEVEL::LO_or_HI:
		return " ANY";
	}
	return "??";
}

enum class PULSE_TYPE : uint8_t {
	UNKNOWN = 0,
	SYCH_PULSE,
	SYNCH_FIRST_PULSE,
	SYNCH_SECOND_PULSE,

	DATA_LOGICAL_00,
	DATA_LOGICAL_01,
};

struct PulseTypes {
	PULSE_TYPE mPulseTypeSynch;
	PULSE_TYPE mPulseTypeData;
};

class Pulse {
public:
	using duration_t = size_t;
private:
	duration_t mUsecDuration;
	PULSE_LEVEL mPulseLevel;

public:
	inline Pulse()
		: mUsecDuration(0), mPulseLevel(PULSE_LEVEL::UNKNOWN){
	}

	inline Pulse(duration_t duration, const PULSE_LEVEL& pulseLevel)
		: mUsecDuration(duration), mPulseLevel(pulseLevel){
	}

	template<typename T>
	inline Pulse(const T duration, const PULSE_LEVEL& pulseLevel)
		// limit duration to maximum value of duration_t
		: mUsecDuration(duration > INT_TRAITS<duration_t>::MAX ?
				INT_TRAITS<duration_t>::MAX  : duration)
		, mPulseLevel(pulseLevel){
	}

	inline void setDuration(const duration_t duration) {
		mUsecDuration = duration;
	}

	template<typename T>
	inline void setDuration(const T duration) {
		// limit duration to maximum value of duration_t
		mUsecDuration = duration > INT_TRAITS<duration_t>::MAX ?
				INT_TRAITS<duration_t>::MAX  : duration;
	}

	inline duration_t getDuration() const {
		return mUsecDuration;
	}

	inline void setLevel(PULSE_LEVEL level) {
		mPulseLevel = level;
	}

	inline PULSE_LEVEL getLevel() const {
		return mPulseLevel;
	}

	bool isDurationInRange(size_t value, unsigned percentTolerance) const {
		// size_t is 16 bit on avr. So static cast to uint32_t avoids
		// temporary overflow when multiplying with 100
		if (static_cast<uint32_t>(mUsecDuration)
				< ((static_cast<uint32_t>(value)
						* (100 - percentTolerance)) / 100)) {
			return false;
		}

		// size_t is 16 bit on avr. So static cast to uint32_t avoids
		// temporary overflow when multiplying with 100
		if (static_cast<uint32_t>(mUsecDuration)
				>= ((static_cast<uint32_t>(value)
						* (100 + percentTolerance)) / 100)) {
			return false;
		}
		return true;
	}
};

class PulseCategory {
	// mPulse holds the level and the average of all pulses that constitute this category.
	Pulse  mPulse;
	size_t usecMinDuration;
	size_t usecMaxDuration;
	size_t pulseCount;

public:
	PulseCategory()
		: mPulse{0, PULSE_LEVEL::UNKNOWN}
		, usecMinDuration(0)
		, usecMaxDuration(0)
		, pulseCount(0)
	{
	}

	PulseCategory(const Pulse& pulse)
		: mPulse{pulse.getDuration(), pulse.getLevel()}
		, usecMinDuration(pulse.getDuration())
		, usecMaxDuration(pulse.getDuration())
		, pulseCount(1)
	{
	}

	inline PULSE_LEVEL getPulseLevel() const {
		return mPulse.getLevel();
	}

	/**
	 * Get the average of the duration of all pulses.
	 */
	inline size_t getWeightedAverage() const {
		return mPulse.getDuration();
	}

	/**
	 * Get the average of the minimum and maximum duration.
	 */
	inline size_t getMinMaxAverage() const {
		return (usecMaxDuration + usecMinDuration) / 2;
	}

	/**
	 * Return the deviation of the minimum and the maximum from MinMax average.
	 * Note that those values are identical.
	 */
	inline unsigned getPercentMinMaxDeviation() const {
		return 100 * (usecMaxDuration - getMinMaxAverage()) / getMinMaxAverage();
	}

	inline bool isValid() const {
		return pulseCount > 0 &&
				getPulseLevel() != PULSE_LEVEL::UNKNOWN;
	}

	void invalidate() {
		pulseCount = 0;
		mPulse.setLevel(PULSE_LEVEL::UNKNOWN);
		mPulse.setDuration(0);
		usecMinDuration = INT_TRAITS<typeof(usecMinDuration)>::MAX;
		usecMaxDuration = 0;
	}

	bool addPulse(const Pulse &pulse) {
		bool result = true;

		// Refresh average for the pulse duration and store it.
		mPulse.setDuration(
				( (static_cast<uint32_t>(getWeightedAverage()) * pulseCount) + pulse.getDuration())
					/ (pulseCount + 1));

		if(pulse.getDuration() < usecMinDuration) {
			usecMinDuration = pulse.getDuration();
		}

		if(pulse.getDuration() > usecMaxDuration) {
			usecMaxDuration = pulse.getDuration();
		}

		if(getPulseLevel() == PULSE_LEVEL::UNKNOWN) {
			mPulse.setLevel(pulse.getLevel());
		} else {
			result = (getPulseLevel() == pulse.getLevel());
		}

		++pulseCount;

		return result;
	}

	void merge(PulseCategory& result, const PulseCategory& other) const {
		result.mPulse.setLevel(getPulseLevel() == other.getPulseLevel() ? getPulseLevel() : PULSE_LEVEL::LO_or_HI);
		result.pulseCount = pulseCount + other.pulseCount;
		result.mPulse.setDuration(((pulseCount * getWeightedAverage())
			+ (other.pulseCount * other.getWeightedAverage())) / result.pulseCount);
		result.usecMinDuration = usecMinDuration < other.usecMinDuration ?
				usecMinDuration : other.usecMinDuration;
		result.usecMaxDuration = usecMaxDuration > other.usecMaxDuration ?
				usecMaxDuration : other.usecMaxDuration;
	}

	template <typename T>
	void dump(T& stream, const char* separator) const {
		stream.print("\t");
		printNumWithSeparator(stream, pulseCount, 3, separator);
		printStringWithSeparator(stream, "recordings of", separator);
		printStringWithSeparator(stream, pulseLevelToString(getPulseLevel()), separator);

		stream.print("[");
		stream.print(separator);

		printUsecWithSeparator(stream, usecMinDuration, 5, separator);

		stream.print("..");
		stream.print(separator);

		printUsecWithSeparator(stream, usecMaxDuration, 5, separator);

		stream.print("]");
		stream.print(separator);

		printUsecWithSeparator(stream, getMinMaxAverage(), 5, separator);

		printStringWithSeparator(stream, "+-", separator);
		printPercentWithSeparator(stream, getPercentMinMaxDeviation(), 2, separator);

		stream.println();
	}

};

} //  namespace RcSwitch

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_HPP_ */
