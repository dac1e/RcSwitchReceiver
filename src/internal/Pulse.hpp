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

#include "Common.hpp"

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

struct Pulse {
	size_t mMicroSecDuration;
	PULSE_LEVEL mPulseLevel;

	bool isDurationInRange(size_t value, unsigned percentTolerance) const {
		// size_t is 16 bit on avr. So static cast to uint32_t avoids
		// temporary overflow when multiplying with 100
		if (static_cast<uint32_t>(mMicroSecDuration)
				< ((static_cast<uint32_t>(value)
						* (100 - percentTolerance)) / 100)) {
			return false;
		}

		// size_t is 16 bit on avr. So static cast to uint32_t avoids
		// temporary overflow when multiplying with 100
		if (static_cast<uint32_t>(mMicroSecDuration)
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
	size_t microSecMinDuration;
	size_t microSecMaxDuration;
	size_t pulseCount;

public:
	PulseCategory()
		: mPulse{0, PULSE_LEVEL::UNKNOWN}
		, microSecMinDuration(0)
		, microSecMaxDuration(0)
		, pulseCount(0)
	{
	}

	PulseCategory(const Pulse& pulse)
		: mPulse{pulse.mMicroSecDuration, pulse.mPulseLevel}
		, microSecMinDuration(pulse.mMicroSecDuration)
		, microSecMaxDuration(pulse.mMicroSecDuration)
		, pulseCount(1)
	{
	}

	inline PULSE_LEVEL getPulseLevel() const {
		return mPulse.mPulseLevel;
	}

	/**
	 * Get the average of the duration of all pulses.
	 */
	inline size_t getWeightedAverage() const {
		return mPulse.mMicroSecDuration;
	}

	/**
	 * Get the average of the minimum and maximum duration.
	 */
	inline size_t getMinMaxAverage() const {
		return (microSecMaxDuration + microSecMinDuration) / 2;
	}

	/**
	 * Return the deviation of the minimum and the maximum from MinMax average.
	 * Note that those values are identical.
	 */
	inline unsigned getPercentMinMaxDeviation() const {
		return 100 * (microSecMaxDuration - getMinMaxAverage()) / getMinMaxAverage();
	}

	inline bool isValid() const {
		return pulseCount > 0 &&
				getPulseLevel() != PULSE_LEVEL::UNKNOWN;
	}

	bool invalidate() {
		return pulseCount = 0;
		mPulse.mPulseLevel = PULSE_LEVEL::UNKNOWN;
		mPulse.mMicroSecDuration = 0;
		microSecMinDuration = static_cast<size_t>(-1);
		microSecMaxDuration = 0;
	}

	bool addPulse(const Pulse &pulse) {
		bool result = true;

		// Refresh average for the pulse duration and store it.
		mPulse.mMicroSecDuration =
				( (static_cast<uint32_t>(getWeightedAverage()) * pulseCount) + pulse.mMicroSecDuration)
					/ (pulseCount + 1);

		if(pulse.mMicroSecDuration < microSecMinDuration) {
			microSecMinDuration = pulse.mMicroSecDuration;
		}

		if(pulse.mMicroSecDuration > microSecMaxDuration) {
			microSecMaxDuration = pulse.mMicroSecDuration;
		}

		if(getPulseLevel() == PULSE_LEVEL::UNKNOWN) {
			mPulse.mPulseLevel = pulse.mPulseLevel;
		} else {
			result = (getPulseLevel() == pulse.mPulseLevel);
		}

		++pulseCount;

		return result;
	}

	void merge(PulseCategory& result, const PulseCategory& other) const {
		result.mPulse.mPulseLevel = getPulseLevel() == other.getPulseLevel() ? getPulseLevel() : PULSE_LEVEL::LO_or_HI;
		result.pulseCount = pulseCount + other.pulseCount;
		result.mPulse.mMicroSecDuration = ((pulseCount * getWeightedAverage())
			+ (other.pulseCount * other.getWeightedAverage())) / result.pulseCount;
		result.microSecMinDuration = microSecMinDuration < other.microSecMinDuration ?
				microSecMinDuration : other.microSecMinDuration;
		result.microSecMaxDuration = microSecMaxDuration > other.microSecMaxDuration ?
				microSecMaxDuration : other.microSecMaxDuration;
	}

	template <typename T>
	void dump(T& stream, const char* separator) const {
		stream.print("\t");
		{
			char buffer[16];
			sprintUint(&buffer[0], pulseCount, 3);
			stream.print(buffer);
		}
		stream.print(separator);
		stream.print(" recordings of");
		stream.print(separator);
		stream.print(" ");

		{
			const char* const levelText = pulseLevelToString(getPulseLevel());
			stream.print(levelText);
		}
		stream.print(separator);
		stream.print(" [");
		stream.print(separator);
		{
			char buffer[16];
			sprintUint(&buffer[0], microSecMinDuration, 5);
			stream.print(buffer);
		}

		stream.print(separator);
		stream.print("us");
		stream.print(" ");

		stream.print("..");
		stream.print(separator);
		{
			char buffer[16];
			sprintUint(&buffer[0], microSecMaxDuration, 5);
			stream.print(buffer);
		}

		stream.print(separator);
		stream.print("us]");
		stream.print(separator);
		stream.print(" ");

		{
			char buffer[16];
			sprintUint(&buffer[0], getMinMaxAverage(), 5);
			stream.print(buffer);
		}

		stream.print(separator);
		stream.print("us");
		stream.print(" ");

		stream.print("+-");
		stream.print(separator);

		{
			char buffer[16];
			sprintUint(&buffer[0], getPercentMinMaxDeviation(), 2);
			stream.print(buffer);
			stream.print("%");
		}

		stream.print(" ");
		stream.print(separator);

		stream.println();
	}

};

} //  namespace RcSwitch

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_HPP_ */
