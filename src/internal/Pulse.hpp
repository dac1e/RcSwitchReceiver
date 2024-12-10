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

#include "ISR_ATTR.hpp"
#include "TypeTraits.hpp"
#include "FormattedPrint.hpp"

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
	case PULSE_LEVEL::UNKNOWN:
		return "UNKNOWN";
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
	/**
	 * unsigned int will be 16 bit on 8 and 16 bit systems and 32 bit
	 * on 32 bit systems.
	 * Typical pulse durations from remote controls are in the
	 * range of 200us .. 15000us. Smaller and longer pulses might
	 * come form HF noise. Hence 16 bit width is sufficient.
	 */
	using duration_t = unsigned int;

private:
	duration_t mUsecDuration;
	PULSE_LEVEL mPulseLevel;

public:
	inline Pulse()
		: mUsecDuration(0), mPulseLevel(PULSE_LEVEL::UNKNOWN){
	}

	/**
	 * When duration_t is identical to the type passed
	 * by the caller, we can just take over everything.
	 * This will be the case on 32 bit systems, where
	 * duration_t = unsigned int and micros() return type is
	 * uint32_t.
	 */
	TEXT_ISR_ATTR_1_INLINE Pulse(duration_t duration, const PULSE_LEVEL &pulseLevel);

	inline void setDuration(const duration_t duration) {
		mUsecDuration = duration;
	}

	/**
	 * When duration_t is smaller than the type passed
	 * by the caller, we limit it to the maximum of
	 * duration_t. This will be the case on 16 bit systems
	 * where unsigned int = duration_t = uint16_t and the
	 * return type of micros() is uint32_t.
	 */
	template<typename T>
	TEXT_ISR_ATTR_1 inline Pulse(const T duration, const PULSE_LEVEL& pulseLevel)
		// limit duration to maximum value of duration_t
		: mUsecDuration(duration > INT_TRAITS<duration_t>::MAX ?
				INT_TRAITS<duration_t>::MAX  : duration)
		, mPulseLevel(pulseLevel){
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

	bool isDurationInRange(unsigned int value, unsigned percentTolerance) const;
};

class PulseCategory {
	/**
	 * mPulse holds the level and the average of all pulses that constitute
	 * this category.
	 */
	Pulse  mPulse;
	unsigned int usecMinDuration;
	unsigned int usecMaxDuration;
	unsigned int pulseCount;

public:
	PulseCategory();

	PulseCategory(const Pulse &pulse);

	inline PULSE_LEVEL getPulseLevel() const {
		return mPulse.getLevel();
	}

	/**
	 * Get the average of the duration of all pulses.
	 */
	inline unsigned int getWeightedAverage() const {
		return mPulse.getDuration();
	}

	/**
	 * Get the average of the minimum and maximum duration.
	 */
	inline unsigned int getMinMaxAverage() const {
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

	void invalidate();

	bool addPulse(const Pulse &pulse);

	void merge(PulseCategory &result, const PulseCategory &other) const;

	template <typename T> void dump(T& serial, const char* separator) const;
};

} //  namespace RcSwitch

#if not defined(ESP32) && not defined(ESP8266)
#include "Pulse.inc"
#endif

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_HPP_ */
