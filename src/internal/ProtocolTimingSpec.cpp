/*
 * ProtocolTimingSpec.cpp
 *
 *  Created on: 23.11.2024
 *      Author: Wolfgang
 */

#include <stddef.h>
#include <stdlib.h>
#include "Common.hpp"
#include "ProtocolTimingSpec.hpp"

namespace {

size_t sprintRange(char *string, const size_t begin, const size_t end, const size_t width) {
	size_t i=0;
	string[i++] = '[';
	RcSwitch::sprintNum(&string[i], begin, width); // adds a null terminated string
	strcat(string, "..");
	i = strlen(string);
	RcSwitch::sprintNum(&string[i], end, width); // adds a null terminated string
	i = strlen(string);
	string[i++] = ']';
	string[i] = '\0'; // ensure null termination
	return i; // return string length
}

inline size_t sprintTimeRange(char* string, const RcSwitch::TimeRange& timeRange, size_t width) {
	return sprintRange(string, timeRange.lowerBound, timeRange.upperBound, width);
}

inline size_t sprintTimeRanges(char* string, const RcSwitch::RxPulsePairTimeRanges& timeRanges, const size_t widthA, const size_t widthB) {
	size_t i=0;
	string[i++] = '{';
	i += sprintTimeRange(&string[i], timeRanges.durationA, widthA); // adds a null terminated string
	i += sprintTimeRange(&string[i], timeRanges.durationB, widthB); // adds a null terminated string
	string[i++] = '}';
	string[i] = '\0'; // ensure null termination
	return i;
}

} // anonymous name space

namespace RcSwitch {

namespace Debug {

void dumpRxTimingSpecTable(serial_t &serial, const rxTimingSpecTable &rxtimingSpecTable) {

	serial.println(" #,i,{<--------SYNCH----------->}{<--------DATA 0-------->}{<--------DATA 1-------->}");
	serial.println("      [  PulseA  ][   PulseB   ]  [  PulseA  ][  PulseB  ]  [  PulseA  ][  PulseB  ]");
	char buffer[96];

	for (size_t i = 0; i < rxtimingSpecTable.size; i++) {
		const RxTimingSpec &p = rxtimingSpecTable.start[i];
		sprintNum(buffer, p.protocolNumber, 2);
		serial.print(buffer);
		if (p.bInverseLevel) {
			serial.print(",1,");
		} else {
			serial.print(",0,");
		}

		sprintTimeRanges(buffer, p.synchronizationPulsePair, 4, 5);
		serial.print(buffer);
		sprintTimeRanges(buffer, p.data0pulsePair, 4, 4);
		serial.print(buffer);
		sprintTimeRanges(buffer, p.data1pulsePair, 4, 4);
		serial.println(buffer);
	}
}

} // namespace Debug
} // namespace RcSwitch
