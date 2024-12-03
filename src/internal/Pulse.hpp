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

#include "Common.hpp"

namespace RcSwitch {

enum class PULSE_LEVEL {
	UNKNOWN = 0,
	LO,
	HI,
};

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
};

struct PulseCategory {
	PULSE_LEVEL pulseLevel;
	size_t microSecDuration;
	size_t microSecMinDuration;
	size_t microSecMaxDuration;
	size_t pulseCount;

	inline void addPulse(const Pulse &pulse) {
		// Refresh average for the pulse duration and store it.
		microSecDuration =
				( (static_cast<uint32_t>(microSecDuration) * pulseCount) + pulse.mMicroSecDuration)
					/ (pulseCount + 1);

		if(pulse.mMicroSecDuration < microSecMinDuration) {
			microSecMinDuration = pulse.mMicroSecDuration;
		}

		if(pulse.mMicroSecDuration > microSecMaxDuration) {
			microSecMaxDuration = pulse.mMicroSecDuration;
		}

		++pulseCount;
	}

	inline bool operator < (const PulseCategory& other) const {
		if(microSecDuration < other.microSecDuration) {return true;}
		if(pulseLevel < other.pulseLevel) {return true;}
		return false;
	}
};

} //  namespace RcSwitch

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_HPP_ */
