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

#ifndef RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER_HPP_

#include <stddef.h>

#include "Pulse.hpp"
#include "ISR_ATTR.hpp"
#include "Container.hpp"

namespace RcSwitch {

static constexpr size_t MAX_PULSE_CATEGORIES = 6;

/**
 * The PulseAnalyzer sorts pulses into duration categories and counts how many
 * pulses belong to this category.
 */
class PulseAnalyzer : public StackBuffer<PulseCategory, MAX_PULSE_CATEGORIES> {
	using baseClass = StackBuffer<PulseCategory, MAX_PULSE_CATEGORIES>;
	const size_t mPercentTolerance;

	bool isOfCategorie(const PulseCategory &category, const Pulse &pulse) const;
	size_t findCategory(const Pulse &pulse) const;

public:
	PulseAnalyzer(size_t percentTolerance = 20);
	bool addPulse(const Pulse &pulse);
	inline void reset() {baseClass::reset();}
	void sort();
	template <typename T>
	void dump(T& stream, char separator) {
		stream.println("Identified pulse categories: ");

		for(size_t i = 0; i < size(); i++) {

			stream.print("\t");
			{
				const char* levelText = (at(i).pulseLevel == PULSE_LEVEL::LO ? " LOW" : "HIGH");
				stream.print(levelText);
			}
			stream.print(separator);
			stream.print(" ");

			{
				char buffer[16];
				sprintUint(&buffer[0], at(i).pulseCount, 3);
				stream.print(buffer);
			}
			stream.print(" times");
			stream.print(separator);
			stream.print(" ");

			stream.print("average: ");
			{
				char buffer[16];
				sprintUint(&buffer[0], at(i).microSecDuration, 5);
				stream.print(buffer);
			}

			stream.print("us");
			stream.print(separator);
			stream.print(" ");

			stream.print("min: ");
			{
				char buffer[16];
				sprintUint(&buffer[0], at(i).microSecMinDuration, 5);
				stream.print(buffer);
			}

			stream.print("us");
			stream.print(separator);
			stream.print(" ");

			stream.print("max: ");
			{
				char buffer[16];
				sprintUint(&buffer[0], at(i).microSecMaxDuration, 5);
				stream.print(buffer);
			}

			stream.print("us");

			stream.println();
		}
	}
};

} /* namespace RcSwitch */

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER_HPP_ */
