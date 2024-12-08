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

	/**
	 * Special encoding of Pulse here in order to save static memory.
	 * This is important on CPUs with little RAM like on
	 * Arduino UNO R3 with ATmega328P.
	 */
	duration_t mUsecInteruptDuration: INT_TRAITS<duration_t>::WIDTH-1;
	duration_t mPulseLevel:1;
	duration_t mPulseDuration;
public:
	TEXT_ISR_ATTR_1 inline TraceRecord()
		: mUsecInteruptDuration(0), mPulseLevel(0), mPulseDuration(0) {
	}

	TEXT_ISR_ATTR_1 inline TraceRecord(const Pulse& pulse, const duration_t usecInterruptDuration)
		: mUsecInteruptDuration(usecInterruptDuration)
		, mPulseLevel(pulse.getLevel() == PULSE_LEVEL::LO ? 0 : 1)
		, mPulseDuration(pulse.getDuration()){
	}

	TEXT_ISR_ATTR_1 inline duration_t getInterruptDuration() const {
		return mUsecInteruptDuration;
	}

	TEXT_ISR_ATTR_1 inline Pulse getPulse() const {
		return Pulse(mPulseDuration, mPulseLevel ? PULSE_LEVEL::HI : PULSE_LEVEL::LO);
	}

	TEXT_ISR_ATTR_1 inline void set(duration_t pulseDuration, PULSE_LEVEL pulseLevel,
			const duration_t usecInterruptDuration) {
		mUsecInteruptDuration = usecInterruptDuration;
		mPulseLevel = pulseLevel ==  PULSE_LEVEL::LO ? 0 : 1;
		mPulseDuration = pulseDuration;
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
		uint32_t interruptLoadSum = 0;
		uint32_t pulseDurationSum = 0;

		const size_t n = baseClass::size();
		size_t i = 0;
		const size_t indexWidth = digitCount(PULSE_TRACES_COUNT);
		while(i < n) {

			const TraceRecord& traceRecord = at(i);
			stream.print("[");
			printNumWithSeparator(stream, i, indexWidth, "]");
			printStringWithSeparator(stream, "", separator);

			// print pulse type (LOW, HIGH)
			printStringWithSeparator(stream, pulseTypeToString(traceRecord.getPulse()), separator);
			printStringWithSeparator(stream, "for", separator);
			printUsecWithSeparator(stream, traceRecord.getPulse().getDuration(), 5, separator);

			printStringWithSeparator(stream, "CPU interrupt load =", separator);
			printUsecWithSeparator(stream, traceRecord.getInterruptDuration(), 3, separator);

			printRatioAsPercentWithSeparator(stream, traceRecord.getInterruptDuration()
					, traceRecord.getPulse().getDuration(), 2, separator);
			stream.println();

			interruptLoadSum += traceRecord.getInterruptDuration();
			pulseDurationSum += traceRecord.getPulse().getDuration();

			i++;
		}

		printStringWithSeparator(stream, "Average CPU interrupt load =", separator);
		printRatioAsPercentWithSeparator(stream, interruptLoadSum/n, pulseDurationSum/n, 2, separator);
		stream.println();
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
