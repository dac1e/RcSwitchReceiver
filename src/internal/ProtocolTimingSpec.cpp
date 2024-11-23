/*
 * ProtocolTimingSpec.cpp
 *
 *  Created on: 23.11.2024
 *      Author: Wolfgang
 */

#include "ProtocolTimingSpec.inc"
#include <UartClass.h>

namespace RcSwitch {

namespace Debug {

void dumpRxTimingSpecTable(UARTClass &serial,
		const std::pair<const RxTimingSpec*, size_t> &rxtimingSpecTable) {

	for (size_t i = 0; i < rxtimingSpecTable.second; i++) {
		const RxTimingSpec &p = rxtimingSpecTable.first[i];
		if (p.protocolNumber < 10) {
			serial.print(' ');
		}
		serial.print(p.protocolNumber);
		if (!p.bInverseLevel) {
			serial.print(", normal");
		} else {
			serial.print(",inverse");
		}
		serial.print(",SY:[");
		serial.print(p.synchronizationPulsePair.durationA.lowerBound);
		serial.print("..");
		serial.print(p.synchronizationPulsePair.durationA.upperBound);
		serial.print("],[");
		serial.print(p.synchronizationPulsePair.durationB.lowerBound);
		serial.print("..");
		serial.print(p.synchronizationPulsePair.durationB.upperBound);
		serial.print("],D0:[");
		serial.print(p.data0pulsePair.durationA.lowerBound);
		serial.print("..");
		serial.print(p.data0pulsePair.durationA.upperBound);
		serial.print("],[");
		serial.print(p.data0pulsePair.durationB.lowerBound);
		serial.print("..");
		serial.print(p.data0pulsePair.durationB.upperBound);
		serial.print("],D1:[");
		serial.print(p.data1pulsePair.durationA.lowerBound);
		serial.print("..");
		serial.print(p.data1pulsePair.durationA.upperBound);
		serial.print("],[");
		serial.print(p.data1pulsePair.durationB.lowerBound);
		serial.print("..");
		serial.print(p.data1pulsePair.durationB.upperBound);
		serial.println("]");
	}
}

} // namespace Debug
} // namespace RcSwitch
