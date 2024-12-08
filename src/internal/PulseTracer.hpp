/*
 * PulseTracer.hpp
 *
 *  Created on: 03.12.2024
 *      Author: Wolfgang
 */

#ifndef RCSWITCH_RECEIVER_INTERNAL_PULSETRACER_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_PULSETRACER_HPP_

#include "ISR_ATTR.hpp"
#include "Container.hpp"
#include "Pulse.hpp"

namespace RcSwitch {

class TraceRecord {
	using duration_t = Pulse::duration_t;
	duration_t mUsecInteruptDuration;
	Pulse mPulse;

public:
	TEXT_ISR_ATTR_1 inline TraceRecord()
		: mUsecInteruptDuration(0) {
	}

	TEXT_ISR_ATTR_1 inline TraceRecord(const Pulse& pulse, const duration_t usecInterruptDuration)
		: mUsecInteruptDuration(usecInterruptDuration), mPulse(pulse) {
	}

	TEXT_ISR_ATTR_1 inline duration_t getDuration() const {
		return mUsecInteruptDuration;
	}

	TEXT_ISR_ATTR_1 inline Pulse getPulse() const {
		return mPulse;
	}

	TEXT_ISR_ATTR_1 inline void set(duration_t pulseDuration, PULSE_LEVEL pulseLevel,
			const duration_t usecInterruptDuration) {
		mUsecInteruptDuration = usecInterruptDuration;
		mPulse = Pulse(pulseDuration, pulseLevel);
	}
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
			printStringWithSeparator(stream, pulseTypeToString(traceRecord.getPulse()), separator);
			printStringWithSeparator(stream, "for", separator);
			printUsecWithSeparator(stream, traceRecord.getPulse().getDuration(), 5, separator);

			printStringWithSeparator(stream, "Interrupt CPU load =", separator);
			printUsecWithSeparator(stream, traceRecord.getDuration(), 3, separator);

			printRatioAsPercentWithSeparator(stream, traceRecord.getDuration()
					, traceRecord.getPulse().getDuration(), 2, separator);

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
