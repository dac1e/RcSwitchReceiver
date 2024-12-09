/*
 * PulseTracer.hpp
 *
 *  Created on: 03.12.2024
 *      Author: Wolfgang
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
	 * This is important on CPUs with little RAM like on* Arduino UNO R3 with
	 * an ATmega328P.
	 */
	duration_t mUsecInteruptDuration: INT_TRAITS<duration_t>::WIDTH-1;
	duration_t mPulseLevel:1;
	duration_t mPulseDuration;

	static const char* pulseTypeToString(const Pulse& pulse) {
		return pulseLevelToString(pulse.getLevel());
	}

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

	template<typename T>
	void dump(T& serial, const char* separator, const size_t i, const size_t indexWidth) const {
		serial.print("[");
		printNumWithSeparator(serial, i, indexWidth, "]");
		printStringWithSeparator(serial, "", separator);

		// print pulse type (LOW, HIGH)
		printStringWithSeparator(serial, pulseTypeToString(getPulse()), separator);
		printStringWithSeparator(serial, "for", separator);
		printUsecWithSeparator(serial, getPulse().getDuration(), 5, separator);

		printStringWithSeparator(serial, "CPU interrupt load =", separator);
		printUsecWithSeparator(serial, getInterruptDuration(), 3, separator);

		printRatioAsPercentWithSeparator(serial, getInterruptDuration()
				, getPulse().getDuration(), 2, separator);
		serial.println();
	}
};

/**
 * This container stores received pulses for debugging and pulse analysis purpose.
 */
template<size_t PULSE_TRACES_COUNT>
class PulseTracer : public RingBuffer<TraceRecord, PULSE_TRACES_COUNT> {
	using baseClass = RingBuffer<TraceRecord, PULSE_TRACES_COUNT>;
public:
	template<typename T> void dump(T& serial, const char* separator) const {

		uint32_t interruptLoadSum = 0;
		uint32_t pulseDurationSum = 0;

		const size_t n = baseClass::size();
		size_t i = 0;
		const size_t indexWidth = digitCount(PULSE_TRACES_COUNT);
		while(i < n) {

			const TraceRecord& traceRecord = at(i);
			traceRecord.dump(serial, separator, i, indexWidth);
			interruptLoadSum += traceRecord.getInterruptDuration();
			pulseDurationSum += traceRecord.getPulse().getDuration();

			i++;
		}

		printStringWithSeparator(serial, "Average CPU interrupt load =", separator);
		printRatioAsPercentWithSeparator(serial, interruptLoadSum/n, pulseDurationSum/n, 2, separator);
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

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSETRACER_HPP_ */
