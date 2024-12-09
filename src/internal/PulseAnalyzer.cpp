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

#include "FormattedPrint.hpp"
#include "PulseAnalyzer.hpp"
#include <Arduino.h>

namespace RcSwitch {

int comparePulseCategoryByDuration(const void* left, const void* right) {
	const RcSwitch::PulseCategory* a = static_cast<const RcSwitch::PulseCategory*>(left);
	const RcSwitch::PulseCategory* b = static_cast<const RcSwitch::PulseCategory*>(right);

	// This is for ascending order by duration
	if(a->getWeightedAverage() < b->getWeightedAverage()) return -1;
	if(b->getWeightedAverage() < a->getWeightedAverage()) return 1;
	return 0;
}

int comparePulseCategoryByLevel(const void* left, const void* right) {
	const RcSwitch::PulseCategory* a = static_cast<const RcSwitch::PulseCategory*>(left);
	const RcSwitch::PulseCategory* b = static_cast<const RcSwitch::PulseCategory*>(right);

	// This is for ascending order by level
	if(a->getPulseLevel() < b->getPulseLevel()) return -1;
	if(b->getPulseLevel() < a->getPulseLevel()) return 1;
	return 0;
}

template<> void PulseAnalyzer::dumpProposedTimings(typeof(Serial)& stream, uint16_t clock) {
	if(mSynchPulseCategories.isValidSynchPulsePair()) {
		if(mDataPulses.isValid()) {
			stream.println();
			stream.print("makeTimingSpec< #,");
			printNumWithSeparator(stream, clock, 3, ",");
			printNumWithSeparator(stream, mPercentTolerance, 3, ",");
			printNumWithSeparator(stream, mSynchPulseCategories.getDurationSyA(clock), 3, ",");
			printNumWithSeparator(stream, mSynchPulseCategories.getDurationSyB(clock), 4, ",");
			printNumWithSeparator(stream, mDataPulses.getMinMaxAverageD0A(clock), 4, ",");
			printNumWithSeparator(stream, mDataPulses.getMinMaxAverageD0B(clock), 4, ",");
			printNumWithSeparator(stream, mDataPulses.getMinMaxAverageD1A(clock), 4, ",");
			printNumWithSeparator(stream, mDataPulses.getMinMaxAverageD1B(clock), 4, ",");
			stream.print(((mDataPulses.bIsInverseLevel) ? " true" : " false"));
			stream.println(">,");
			stream.println();
			stream.println("-------- Replace the '#' above by a unique identifier ---------");
			stream.println("-Example sketch PrintReceivedData.ino demonstrates application-");
		}
	}
}

template <> void PulseAnalyzer::dump(typeof(Serial)& stream, const char* separator) {
#if false
	stream.println("Identified pulse categories:");
	mAllPulseCategories.dump(stream, separator);
#endif

	if(mSynchPulseCategories.size()) {
		stream.println("\nIdentified SYNCH pulse ranges:");
		mSynchPulseCategories.dump(stream, separator);
	}

#if false
	if(mSynchPulseCategories.overflowCount()) {
		stream.print("Warning! ");
		stream.print(mSynchPulseCategories.overflowCount());
		stream.print(" more categories could not be recorded.");
		stream.print(" Category recording capacity is ");
		stream.print(mSynchPulseCategories.capacity);
		stream.println('.');
	}
#endif

	if(mDataPulseCategories.size()) {
		stream.println("\nIdentified DATA pulse ranges:");
		mDataPulseCategories.dump(stream, separator);
	}

#if false
	if(mDataPulseCategories.overflowCount()) {
		stream.print("Warning! ");
		stream.print(mDataPulseCategories.overflowCount());
		stream.print(" more categories could not be recorded.");
		stream.print(" Category recording capacity is ");
		stream.print(mDataPulseCategories.capacity);
		stream.println('.');
	}
#endif

	const bool bOk = mSynchPulseCategories.isAtTheEdge() && mDataPulseCategories.isAtTheEdge() && mDataPulses.isValid();
	if(bOk) {
#if false
		{
			stream.println("All SHORT DATA pulse categories together:");
			PulseCategory shortPulses;
			mDataPulses.d0A->merge(shortPulses, *mDataPulses.d1B);
			shortPulses.dump(stream, separator);
		}

		{
			stream.println("All LONG DATA pulse categories together:");
			PulseCategory longPulses;
			mDataPulses.d1A->merge(longPulses, *mDataPulses.d0B);
			longPulses.dump(stream, separator);
		}
#endif
		stream.println();
		static const char* const frame =
					   "***************************************************************";
		stream.println(frame);
		stream.println("Protocol detection succeeded. Timing specification proposal:");
		dumpProposedTimings(stream, 10);
		stream.println(frame);
	} else {
		stream.println("\n"
				       "Protocol detection failed. Please try again. You may\n"
					   "reposition your Remote Control a bit or use a different\n"
					   "RC button. Be sure that you press the RC button at\n"
					   "least for 3 seconds, before you start the pulse trace.");
	}
}

PulseAnalyzer::PulseAnalyzer(const RingBufferReadAccess<TraceRecord>& input, unsigned percentTolerance)
	:mInput(input)
	,mPercentTolerance(percentTolerance)
{
}

void PulseAnalyzer::buildAllCategories() {
	mAllPulseCategories.reset();
	mAllPulseCategories.build(mInput, mPercentTolerance);
}

void PulseAnalyzer::buildSynchAndDataCategories() {
	mDataPulses.reset();
	mDataPulseCategories.reset();
	mSynchPulseCategories.reset();
	if (mAllPulseCategories.size() >= 0) {
		mDataPulseCategories.build(mDataPulses, mInput, mPercentTolerance, mSynchPulseCategories,
			mAllPulseCategories.at(mAllPulseCategories.size()-1).getWeightedAverage());
	}
}

} /* namespace RcSwitch */
