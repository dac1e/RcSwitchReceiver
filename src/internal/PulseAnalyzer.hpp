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

static constexpr size_t MAX_PULSE_CATEGORIES = 12;

/**
 * The PulseAnalyzer sorts pulses into duration categories and counts how many
 * pulses belong to this category.
 */
class PulseAnalyzer : public StackBuffer<PulseCategory, MAX_PULSE_CATEGORIES> {
	using baseClass = StackBuffer<PulseCategory, MAX_PULSE_CATEGORIES>;

	/**
	 * Indexed access to the pulses ring buffer where the oldest pulse is at index 0.
	 */
	const RingBufferReadAccess<Pulse> mPulses;
	const unsigned mPercentTolerance;
	PulseCategory mCategoryOfSynchPulseA;
	PulseCategory mCategoryOfSynchPulseB;

	const PulseCategory* mDataPulses[2][2];
	bool mInverse;
	static constexpr size_t A = 0;
	static constexpr size_t B = 1;

	bool isOfCategorie(const PulseCategory &category, const Pulse &pulse) const;
	size_t findCategory(const Pulse &pulse) const;

	const PulseCategory& getLowPulseFromPair(size_t begin, bool& bOk)const;
	const PulseCategory& getHighPulseFromPair(size_t begin, bool& bOk) const;
	bool addPulse(size_t i);
	bool buildCategoriesFromPulses();

	/** Sort the categories in descending order of pulse duration */
	void sortDescendingByDuration();
	void invalidateProtocolGuess();

public:
	PulseAnalyzer(const RingBufferReadAccess<Pulse>& pulses, unsigned percentTolerance = 20);
	void reset();

	void analyze();

	template <typename T>
	void dumpPulseCategory(T& stream, const PulseCategory& pulseCategory, const char* separator) {
		stream.print("\t");
		{
			char buffer[16];
			sprintUint(&buffer[0], pulseCategory.pulseCount, 3);
			stream.print(buffer);
		}
		stream.print(" recordings of");
		stream.print(separator);
		stream.print(" ");

		{
			const char* const levelText = pulseLevelToString(pulseCategory.pulseLevel);;
			stream.print(levelText);
		}
		stream.print(separator);
		stream.print(" ");

		{
			char buffer[16];
			sprintUint(&buffer[0], pulseCategory.microSecDuration, 5);
			stream.print(buffer);
		}

		stream.print(separator);
		stream.print("us");
		stream.print(" ");

		stream.print("[");
		stream.print(separator);
		{
			char buffer[16];
			sprintUint(&buffer[0], pulseCategory.microSecMinDuration, 5);
			stream.print(buffer);
		}

		stream.print(separator);
		stream.print("us");
		stream.print(" ");

		stream.print("..");
		stream.print(separator);
		{
			char buffer[16];
			sprintUint(&buffer[0], pulseCategory.microSecMaxDuration, 5);
			stream.print(buffer);
		}

		stream.print(separator);
		stream.print("us]");

		stream.println();
	}

	template <typename T>
	void dump(T& stream, const char* separator) {

		stream.println("Identified pulse categories:");
		if(mCategoryOfSynchPulseA.isValid()) {
			dumpPulseCategory(stream, mCategoryOfSynchPulseA, separator);
			if(mCategoryOfSynchPulseB.isValid()) {
				dumpPulseCategory(stream, mCategoryOfSynchPulseB, separator);
			}
		}

		for(size_t i = 0; i < size(); i++) {
			const PulseCategory& pulseCategory = at(i);
			dumpPulseCategory(stream, pulseCategory, separator);
		}

		if(overflowCount()) {
			stream.print("Warning! ");
			stream.print(overflowCount());
			stream.print(" more categories could not be recorded.");
			stream.print(" Category recording capacity is ");
			stream.print(capacity);
			stream.println('.');
		}

		if(mCategoryOfSynchPulseA.isValid() && mCategoryOfSynchPulseB.isValid()) {
			stream.println("Protocol guess: ");
			stream.print("inverse: ");
			stream.println((mInverse ? "true" : "false"));

			stream.print("SynchA: ");
			dumpPulseCategory(stream, mCategoryOfSynchPulseA, separator);
			stream.print("SynchB: ");
			dumpPulseCategory(stream, mCategoryOfSynchPulseB, separator);

			if(mDataPulses[0][A] && mDataPulses[0][B] && mDataPulses[1][A] && mDataPulses[1][B]) {
				stream.print("data0_A: ");
				dumpPulseCategory(stream, *mDataPulses[0][A], separator);
				stream.print("data0_B: ");
				dumpPulseCategory(stream, *mDataPulses[0][B], separator);
				stream.print("data1_A: ");
				dumpPulseCategory(stream, *mDataPulses[1][A], separator);
				stream.print("data1_B: ");
				dumpPulseCategory(stream, *mDataPulses[1][B], separator);
			}
		}


	}
};

} /* namespace RcSwitch */

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER_HPP_ */
