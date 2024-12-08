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
	Pulse::duration_t mUsecInteruptDuration;
};

/**
 * This container stores received pulses for debugging and pulse analysis purpose.
 */
template<size_t PULSE_TRACES_COUNT>
class PulseTracer : public RingBuffer<TraceRecord, PULSE_TRACES_COUNT> {
	using baseClass = RingBuffer<TraceRecord, PULSE_TRACES_COUNT>;
public:
	static const char* pulseTypeToString(const Pulse& pulse) {
		return pulseLevelToString(pulse.getLevel());
	}

	template<typename T> void dump(T& stream, const char* separator) const {

#if false
		stream.println(INT_TRAITS<int8_t>::MIN);
		stream.println(INT_TRAITS<int8_t>::MAX);
		stream.println(INT_TRAITS<uint8_t>::MIN);
		stream.println(INT_TRAITS<uint8_t>::MAX);
		stream.println();
		stream.println(INT_TRAITS<int16_t>::MIN);
		stream.println(INT_TRAITS<int16_t>::MAX);
		stream.println(INT_TRAITS<uint16_t>::MIN);
		stream.println(INT_TRAITS<uint16_t>::MAX);
		stream.println();
		stream.println(INT_TRAITS<int32_t>::MIN);
		stream.println(INT_TRAITS<int32_t>::MAX);
		stream.println(INT_TRAITS<uint32_t>::MIN);
		stream.println(INT_TRAITS<uint32_t>::MAX);
		stream.println();
		stream.println(INT_TRAITS<size_t>::MIN);
		stream.println(INT_TRAITS<size_t>::MAX);
		stream.println();
#endif

		const size_t n = baseClass::size();
		size_t i = 0;
		const size_t indexWidth = digitCount(PULSE_TRACES_COUNT);
		while(i < n) {
			// Start with index one, because the interrupt duration to be printed needs to
			// be taken from the previous entry.
			const TraceRecord& traceRecord = at(i);
			stream.print("[");
			printNumWithSeparator(stream, i, indexWidth, "]");
			printStringWithSeparator(stream, "", separator);

			// print pulse type (LOW, HIGH)
			printStringWithSeparator(stream, pulseTypeToString(traceRecord.mPulse), separator);
			printStringWithSeparator(stream, "for", separator);
			printUsecWithSeparator(stream, traceRecord.mPulse.getDuration(), 5, separator);

			printStringWithSeparator(stream, "Interrupt duration", separator);
			printUsecWithSeparator(stream, traceRecord.mUsecInteruptDuration, 3, separator);

			stream.println();
			i++;
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
