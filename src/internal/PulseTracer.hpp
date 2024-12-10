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

#ifndef RCSWITCH_RECEIVER_INTERNAL_PULSETRACER_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_PULSETRACER_HPP_

#include "ISR_ATTR.hpp"
#include "RcSwitchContainer.hpp"
#include "Pulse.hpp"
#include "TypeTraits.hpp"

namespace RcSwitch {

class TraceRecord {
	using duration_t = Pulse::duration_t;

	/**
	 * There is a special encoding of Pulse here in order to save static memory.
	 * This is important on CPUs with little RAM like on Arduino UNO R3 with
	 * an ATmega328P.
	 */
	duration_t mUsecInteruptDuration: INT_TRAITS<duration_t>::WIDTH-1;
	duration_t mPulseLevel:1;
	duration_t mPulseDuration;

	static const char* pulseTypeToString(const Pulse& pulse) {
		return pulseLevelToString(pulse.getLevel());
	}

public:
	TEXT_ISR_ATTR_1_INLINE TraceRecord();

	TEXT_ISR_ATTR_1_INLINE TraceRecord(const Pulse &pulse, const duration_t usecInterruptDuration);

	TEXT_ISR_ATTR_1_INLINE void set(duration_t pulseDuration, PULSE_LEVEL pulseLevel,
			const duration_t usecInterruptDuration);

	inline duration_t getInterruptDuration() const {
		return mUsecInteruptDuration;
	}

	inline Pulse getPulse() const {
		return Pulse(mPulseDuration, mPulseLevel ? PULSE_LEVEL::HI : PULSE_LEVEL::LO);
	}

	template<typename T>
	void dump(T& serial, const char* separator, const size_t i, const size_t indexWidth) const;
};

/**
 * This container stores received pulses for debugging and pulse analysis purpose.
 */
template<size_t PULSE_TRACES_COUNT>
class PulseTracer : public RingBuffer<TraceRecord, PULSE_TRACES_COUNT> {
	using baseClass = RingBuffer<TraceRecord, PULSE_TRACES_COUNT>;
public:
	template<typename T> void dump(T& serial, const char* separator) const {

		const size_t n = baseClass::size();
		if(n > 0) {
			uint32_t interruptLoadSum = 0;
			uint32_t pulseDurationSum = 0;

			size_t i = 0;
			const size_t indexWidth = decimalDigits(PULSE_TRACES_COUNT);
			while(i < n) {
				const TraceRecord& traceRecord = at(i);
				traceRecord.dump(serial, separator, i, indexWidth);
				interruptLoadSum += traceRecord.getInterruptDuration();
				pulseDurationSum += traceRecord.getPulse().getDuration();
				i++;
			}

			printStringWithSeparator(serial, "Average CPU interrupt load =", separator);
			printRatioAsPercentWithSeparator(serial, interruptLoadSum/n, pulseDurationSum/n, 2, separator);
		} else {
			serial.println("No pulses found.");
		}
		serial.println();
	}

	PulseTracer() {
	}

	/**
	 * Remove all pulses from this pulse tracer container.
	 */
	using baseClass::reset; // Just  make the base class reset() method public.

	/**
	 * Get the actual number of pulses within this pulse tracer.
	 */
	using baseClass::size;

	/**
	 * Get a pulse located at a particular index.
	 */
	using baseClass::at;
};
} // namespace RcSwitch

#if not defined(ESP32) && not defined(ESP8266)
#include "PulseTracer.inc"
#endif

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSETRACER_HPP_ */
