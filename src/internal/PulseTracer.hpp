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


struct TraceElement {
	uint32_t mUsecInterruptEntry;
	uint32_t mUsecInterruptExit;
	Pulse mPulse;
	uint32_t mUsecLastInterruptEntry;
};

/**
 * This container stores received pulses for debugging and pulse analysis purpose.
 */
template<size_t PULSE_TRACES_COUNT>
class PulseTracer : public RingBuffer<TraceElement, PULSE_TRACES_COUNT> {
	using baseClass = RingBuffer<TraceElement, PULSE_TRACES_COUNT>;
public:
	using traceElement_t = TraceElement;

	static const char* pulseTypeToString(const Pulse& pulse) {
		return pulseLevelToString(pulse.mPulseLevel);
	}

	template<typename T> void dump(T& stream, const char* separator) const {
		const size_t n = baseClass::size();
		size_t i = 0;
		const size_t indexWidth = digitCount(PULSE_TRACES_COUNT);
		while(i < n) {
			const Pulse& pulse = at(i).mPulse;
			// print trace buffer index
			{
				char buffer[16];
				buffer[0] = ('[');
				sprintUint(&buffer[1], i, indexWidth);
				stream.print(buffer);
				stream.print("]");
			}
			stream.print(separator);
			stream.print(" ");

			// print pulse type (LOW, HIGH)
			stream.print(pulseTypeToString(pulse));
			stream.print(separator);
			stream.print(" ");
			stream.print("for");
			stream.print(separator);
			stream.print(" ");

			// print pulse length
			{
				char buffer[16];
				sprintUint(buffer, pulse.mMicroSecDuration, 6);
				stream.print(buffer);
			}
			stream.print(separator);
			stream.println("us");

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
