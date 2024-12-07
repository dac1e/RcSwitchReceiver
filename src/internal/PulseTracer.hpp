/*
 * PulseTracer.hpp
 *
 *  Created on: 03.12.2024
 *      Author: Wolfgang
 */

#ifndef RCSWITCH_RECEIVER_INTERNAL_PULSETRACER_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_PULSETRACER_HPP_

#include "Container.hpp"
#include "Pulse.hpp"

namespace RcSwitch {

struct TraceRecord {
	Pulse mPulse;
};

/**
 * This container stores received pulses for debugging and pulse analysis purpose.
 */
template<size_t PULSE_TRACES_COUNT>
class PulseTracer : public RingBuffer<TraceRecord, PULSE_TRACES_COUNT> {
	using baseClass = RingBuffer<TraceRecord, PULSE_TRACES_COUNT>;
public:
	static const char* pulseTypeToString(const Pulse& pulse) {
		return pulseLevelToString(pulse.mPulseLevel);
	}

	template<typename T> void dump(T& stream, const char* separator) const {
		const size_t n = baseClass::size();
		size_t i = 0;
		const size_t indexWidth = digitCount(PULSE_TRACES_COUNT);
		while(i < n) {
			const TraceRecord& traceRecord = at(i);
			stream.print('[');
			printUintWithSeparator(stream, i, indexWidth, "]");
			printStringWithSeparator(stream, "", separator);

			// print pulse type (LOW, HIGH)
			printStringWithSeparator(stream, pulseTypeToString(traceRecord.mPulse), separator);
			printStringWithSeparator(stream, "for", separator);
			printUsecWithSeparator(stream, traceRecord.mPulse.mUsecDuration, 6, separator);

			stream.println();
			++i;
		}
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

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSETRACER_HPP_ */
