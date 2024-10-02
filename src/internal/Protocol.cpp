/*
 * Protocol.cpp
 *
 *  Created on: 01.10.2024
 *      Author: Wolfgang
 */

#include "Protocol.hpp"

namespace RcSwitch {


/** Normal level protocol group specification in microseconds: */
static const Protocol normalLevelProtocolsTable[] { // Sorted in ascending order of lowTimeRange.msecLowerBound
		//     |synch                                                    |data
		//                                                                |logical 0 data bit pulse pair                            |logical 1 data bit pulse pair
        //      |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..
        //      |..is the second pulse      |..is the first pulse         |..is the second pulse      |..is the first pulse         |..is the second pulse      |..is the first pulse
		//#p    |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound
//		{    7,{{        7440,       11160},{         240,         360}},{{         720,        1080},{         120,         180}},{{         120,         180},{         720,        1080}}},
//		{    1,{{        8680,       13020},{         280,         420}},{{         840,        1260},{         280,         420}},{{         280,         420},{         840,        1260}}},
//		{    4,{{        1824,        2736},{         304,         456}},{{         912,        1368},{         304,         456}},{{         304,         456},{         912,        1368}}},
//		{    8,{{       20800,       31200},{         480,         720}},{{        2560,        3840},{        1120,        1680}},{{        2560,        3840},{         480,         720}}},
//		{    2,{{        5200,        7800},{         520,         780}},{{        1040,        1560},{         520,         780}},{{         520,         780},{        1040,        1560}}},
//		{    5,{{        5600,        8400},{        2400,        3600}},{{         800,        1200},{         400,         600}},{{         400,         600},{         800,        1200}}},
//		{    3,{{        5680,        8520},{        2400,        3600}},{{         880,        1320},{         320,         480}},{{         480,         720},{         720,        1080}}},

		//             #,  %,  clk,  syA,syB,  d0A,d0B,  d1A,d1B
		MakeProtocol<  7, 20,  150,  2,   62,    1,  6,    6,  1>::VALUE, // (HS2303-PT)
		MakeProtocol<  1, 20,  350,  1,   31,    1,  3,    3,  1>::VALUE, // ()
		MakeProtocol<  4, 20,  380,  1,    6,    1,  3,    3,  1>::VALUE, // ()
		MakeProtocol<  8, 20,  200,  3,  130,    7, 16,    3, 16>::VALUE, // (Conrad RS-200 RX)
		MakeProtocol<  2, 20,  650,  1,   10,    1,  3,    3,  1>::VALUE, // ()
		MakeProtocol<  5, 20,  500,  6,   14,    1,  2,    2,  1>::VALUE, // ()
		MakeProtocol<  3, 20,  100, 30,   71,    4, 11,    9,  6>::VALUE, // ()
};

/** Inverse level protocol group specification in microseconds: */
static const Protocol inverseLevelProtocolsTable[] { // Sorted in ascending order of msecHighTimeLowerBound
		//     |synch                                                    |data
		//                                                                |logical 0 data bit pulse pair                            |logical 1 data bit pulse pair
        //      |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..
        //      |..is the first pulse       |..is the second pulse        |..is the first pulse       |..is the second pulse        |..is the first pulse       |..is the second pulse
		//#p    |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound
//		{   12,{{         216,         324},{        7776,       11664}},{{         216,         324},{         432,         648}},{{         432,         648},{         216,         324}}},
//		{   11,{{         256,         384},{        9216,       13824}},{{         256,         384},{         512,         768}},{{         512,         768},{         256,         384}}},
//		{   10,{{         292,         438},{        5256,        7884}},{{         876,        1314},{         292,         438}},{{         292,         438},{         876,        1314}}},
//		{    6,{{         360,         540},{        8280,       12420}},{{         360,         540},{         720,        1080}},{{         720,        1080},{         360,         540}}},
//		{    9,{{        1120,        1680},{       20800,       31200}},{{        2560,        3840},{        1120,        1680}},{{        2560,        3840},{         480,         720}}},

		//             #,  %,  clk,  syA,syB,  d0A,d0B,  d1A,d1B
		MakeProtocol< 12, 20,  320,    1, 36,    1,  2,    2,  1>::VALUE, // (SM5212)
		MakeProtocol< 11, 20,  270,    1, 36,    1,  2,    2,  1>::VALUE, // (HT12E)
		MakeProtocol< 10, 20,  365,    1, 18,    3,  1,    1,  3>::VALUE, // (1ByOne Doorbell)
		MakeProtocol<  6, 20,  450,    1, 23,    1,  2,    2,  1>::VALUE, // (HT6P20B)
};

/* The number of rows of the 2 above tables. */
constexpr size_t  normalLevelProtocolsTableRowCount =
		sizeof( normalLevelProtocolsTable)/sizeof( normalLevelProtocolsTable[0]);
constexpr size_t inverseLevelProtocolsTableRowCount =
		sizeof(inverseLevelProtocolsTable)/sizeof(inverseLevelProtocolsTable[0]);

bool Protocol::isNormalLevelProtocol() const {
	const bool result = (this < &normalLevelProtocolsTable[normalLevelProtocolsTableRowCount]) &&
			(this >= &normalLevelProtocolsTable[0]);
	return result;
}

bool Protocol::isInverseLevelProtocol() const {
	const bool result = this < (&inverseLevelProtocolsTable[inverseLevelProtocolsTableRowCount]) &&
			(this >= &inverseLevelProtocolsTable[0]);
	return result;
}

/** Returns the array of protocols for a protocol group. */
std::pair<const Protocol*, size_t> getProtocolTable(const size_t protocolGroupId) {
	static const std::pair<const Protocol*, size_t> protocolGroups[] = {
			{ normalLevelProtocolsTable,  normalLevelProtocolsTableRowCount},
			{inverseLevelProtocolsTable, inverseLevelProtocolsTableRowCount},
	};

	return protocolGroups[protocolGroupId];
}

#if DEBUG_RCSWITCH_PROTOCOL_DEF

void printProtocolTable(UARTClass& serial, const size_t protocolGroup) {
	std::pair<const Protocol*, size_t> pt = RcSwitch::getProtocolTable(protocolGroup);
	for (size_t i = 0; i < pt.second; i++) {
		const Protocol &p = (pt.first)[i];
		serial.print(p.protocolNumber);
		serial.print(",SY:[");
		serial.print(p.synchronizationPulsePair.durationA.lowerBound);
		serial.print("..");
		serial.print(p.synchronizationPulsePair.durationA.upperBound);
		serial.print("],[");
		serial.print(p.synchronizationPulsePair.durationB.lowerBound);
		serial.print("..");
		serial.print(p.synchronizationPulsePair.durationB.upperBound);
		serial.print("],D0:[");
		serial.print(p.logical0PulsePair.durationA.lowerBound);
		serial.print("..");
		serial.print(p.logical0PulsePair.durationA.upperBound);
		serial.print("],[");
		serial.print(p.logical0PulsePair.durationB.lowerBound);
		serial.print("..");
		serial.print(p.logical0PulsePair.durationB.upperBound);
		serial.print("],D1:[");
		serial.print(p.logical1PulsePair.durationA.lowerBound);
		serial.print("..");
		serial.print(p.logical1PulsePair.durationA.upperBound);
		serial.print("],[");
		serial.print(p.logical1PulsePair.durationB.lowerBound);
		serial.print("..");
		serial.print(p.logical1PulsePair.durationB.upperBound);
		serial.println("]");
	}
}
#endif

} /* namespace RcSwitch */
