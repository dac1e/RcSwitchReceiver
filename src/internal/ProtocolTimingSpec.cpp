/*
 * Protocol.cpp
 *
 *  Created on: 01.10.2024
 *      Author: Wolfgang
 */

#include "ProtocolTimingSpec.hpp"

namespace RcSwitch {

/** Get the first type of a list of types. */
template<typename T, typename ...R> struct firstType {
	typedef T type;
};

/** isGreater<T1,T2> type comparison declaration. */
template<typename T1, typename T2> struct isGreater;

/** isGreater<T1,T2> comparison specialization for protocol timing specification. isGreater<T1,T2>::VALUE is true if T1 > T2. Otherwise false. */
template<
	size_t protocolNumber_T1, size_t percentTolerance_T1, size_t clock_T1, size_t synchA_T1, size_t synchB_T1, size_t data0_A_T1, size_t data0_B_T1, size_t data1_A_T1, size_t data1_B_T1,
	size_t protocolNumber_T2, size_t percentTolerance_T2, size_t clock_T2, size_t synchA_T2, size_t synchB_T2, size_t data0_A_T2, size_t data0_B_T2, size_t data1_A_T2, size_t data1_B_T2
>
struct isGreater <
	makeProtocolTimingSpec<protocolNumber_T1, percentTolerance_T1, clock_T1, synchA_T1, synchB_T1, data0_A_T1, data0_B_T1, data1_A_T1, data1_B_T1>,
	makeProtocolTimingSpec<protocolNumber_T2, percentTolerance_T2, clock_T2, synchA_T2, synchB_T2, data0_A_T2, data0_B_T2, data1_A_T2, data1_B_T2>
>
{
	static constexpr size_t usecSynchA_lowerBound_T1 = makeProtocolTimingSpec<protocolNumber_T1, percentTolerance_T1, clock_T1, synchA_T1, synchB_T1, data0_A_T1, data0_B_T1, data1_A_T1, data1_B_T1>::usecSynchA_lowerBound;
	static constexpr size_t usecSynchA_lowerBound_T2 = makeProtocolTimingSpec<protocolNumber_T2, percentTolerance_T2, clock_T2, synchA_T2, synchB_T2, data0_A_T2, data0_B_T2, data1_A_T2, data1_B_T2>::usecSynchA_lowerBound;

	/* Implement greater: Comparison criteria is usecSynchA_lowerBound. */
	static constexpr bool VALUE = usecSynchA_lowerBound_T1 > usecSynchA_lowerBound_T2;
};


template<bool greater, typename T1, typename ...R> struct SortRxSpec {
	static constexpr bool IS_T1_GREATER = isGreater<T1, typename firstType<R...>::type >::VALUE;
	typename T1::rx_spec_t m=T1::RX;
	SortRxSpec<IS_T1_GREATER, R...> successor;
};

template<typename T1, typename ...R> struct SortRxSpec<true, T1, R...> {
	static constexpr bool IS_T1_GREATER = isGreater<T1, typename firstType<R...>::type >::VALUE;
	SortRxSpec<IS_T1_GREATER, R...> predecessor;
	typename T1::rx_spec_t m=T1::RX;
};

template<bool greater, typename T1> struct SortRxSpec<greater, T1> {
	typename T1::rx_spec_t m=T1::RX;
};

template<typename T1> struct SortRxSpec<true, T1> {
	typename T1::rx_spec_t m=T1::RX;
};

template<typename T1, typename ...R> struct RxTimingSpecTable {
	static constexpr bool IS_T1_GREATER = isGreater<T1, typename firstType<R...>::type >::VALUE;
	SortRxSpec<IS_T1_GREATER, T1, R...> m;
	static constexpr size_t ELEMENT_COUNT = sizeof(RxTimingSpecTable) / sizeof(typename T1::rx_spec_t);

	static const typename T1::rx_spec_t* toArray(const RxTimingSpecTable& rxSpecTable) {
		return reinterpret_cast<const typename T1::rx_spec_t*>(&rxSpecTable.m);
	}

	const typename T1::rx_spec_t* toArray() const {
		return toArray(*this);
	}
};

template<typename T1> struct RxTimingSpecTable<T1> {
	typename T1::rx_spec_t m=T1::RX;
	static constexpr size_t ELEMENT_COUNT = sizeof(RxTimingSpecTable) / sizeof(T1::rx_spec_t);

	static const typename T1::rx_spec_t* toArray(const RxTimingSpecTable& rxSpecTable) {
		return reinterpret_cast<const typename T1::rx_spec_t*>(&rxSpecTable.m);
	}

	const typename T1::rx_spec_t* toArray() const {
		return toArray(*this);
	}
};

///** Normal level protocol group specification in microseconds: */
//static const RxTimingSpec normalLevelProtocolsTable[] { // Sorted in ascending order of lowTimeRange.msecLowerBound
//		//     |synch                                                    |data
//		//                                                                |logical 0 data bit pulse pair                            |logical 1 data bit pulse pair
//        //      |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..
//        //      |..is the second pulse      |..is the first pulse         |..is the second pulse      |..is the first pulse         |..is the second pulse      |..is the first pulse
//		//#p    |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound
////		{    7,{{        7440,       11160},{         240,         360}},{{         720,        1080},{         120,         180}},{{         120,         180},{         720,        1080}}},
////		{    1,{{        8680,       13020},{         280,         420}},{{         840,        1260},{         280,         420}},{{         280,         420},{         840,        1260}}},
////		{    4,{{        1824,        2736},{         304,         456}},{{         912,        1368},{         304,         456}},{{         304,         456},{         912,        1368}}},
////		{    8,{{       20800,       31200},{         480,         720}},{{        2560,        3840},{        1120,        1680}},{{        2560,        3840},{         480,         720}}},
////		{    2,{{        5200,        7800},{         520,         780}},{{        1040,        1560},{         520,         780}},{{         520,         780},{        1040,        1560}}},
////		{    5,{{        5600,        8400},{        2400,        3600}},{{         800,        1200},{         400,         600}},{{         400,         600},{         800,        1200}}},
////		{    3,{{        5680,        8520},{        2400,        3600}},{{         880,        1320},{         320,         480}},{{         480,         720},{         720,        1080}}},
//
//		//             #,  %,  clk,  syA,syB,  d0A,d0B,  d1A,d1B
//		makeProtocolTimingSpec<  7, 20,  150,  2,   62,    1,  6,    6,  1>::RX, // (HS2303-PT)
//		makeProtocolTimingSpec<  1, 20,  350,  1,   31,    1,  3,    3,  1>::RX, // ()
//		makeProtocolTimingSpec<  4, 20,  380,  1,    6,    1,  3,    3,  1>::RX, // ()
//		makeProtocolTimingSpec<  8, 20,  200,  3,  130,    7, 16,    3, 16>::RX, // (Conrad RS-200 RX)
//		makeProtocolTimingSpec<  2, 20,  650,  1,   10,    1,  3,    3,  1>::RX, // ()
//		makeProtocolTimingSpec<  5, 20,  500,  6,   14,    1,  2,    2,  1>::RX, // ()
//		makeProtocolTimingSpec<  3, 20,  100, 30,   71,    4, 11,    9,  6>::RX, // ()
//};
static const RxTimingSpecTable <
//                           #,  %,  clk,  syA,syB,  d0A,d0B,  d1A,d1B
	makeProtocolTimingSpec<  7, 20,  150,  2,   62,    1,  6,    6,  1>, // (HS2303-PT)
	makeProtocolTimingSpec<  1, 20,  350,  1,   31,    1,  3,    3,  1>, // ()
	makeProtocolTimingSpec<  4, 20,  380,  1,    6,    1,  3,    3,  1>, // ()
	makeProtocolTimingSpec<  8, 20,  200,  3,  130,    7, 16,    3, 16>, // (Conrad RS-200 RX)
	makeProtocolTimingSpec<  2, 20,  650,  1,   10,    1,  3,    3,  1>, // ()
	makeProtocolTimingSpec<  5, 20,  500,  6,   14,    1,  2,    2,  1>, // ()
	makeProtocolTimingSpec<  3, 20,  100, 30,   71,    4, 11,    9,  6>  // ()
> normalLevelProtocolsTable;


///** Inverse level protocol group specification in microseconds: */
//static const RxTimingSpec inverseLevelProtocolsTable[] { // Sorted in ascending order of msecHighTimeLowerBound
//		//     |synch                                                    |data
//		//                                                                |logical 0 data bit pulse pair                            |logical 1 data bit pulse pair
//        //      |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..  |low level pulse duration.. |high level pulse duration..
//        //      |..is the first pulse       |..is the second pulse        |..is the first pulse       |..is the second pulse        |..is the first pulse       |..is the second pulse
//		//#p    |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound      |lowerBound  |upperBound    |lowerBound  |upperBound
////		{   12,{{         216,         324},{        7776,       11664}},{{         216,         324},{         432,         648}},{{         432,         648},{         216,         324}}},
////		{   11,{{         256,         384},{        9216,       13824}},{{         256,         384},{         512,         768}},{{         512,         768},{         256,         384}}},
////		{   10,{{         292,         438},{        5256,        7884}},{{         876,        1314},{         292,         438}},{{         292,         438},{         876,        1314}}},
////		{    6,{{         360,         540},{        8280,       12420}},{{         360,         540},{         720,        1080}},{{         720,        1080},{         360,         540}}},
////		{    9,{{        1120,        1680},{       20800,       31200}},{{        2560,        3840},{        1120,        1680}},{{        2560,        3840},{         480,         720}}},
//
//		//             #,  %,  clk,  syA,syB,  d0A,d0B,  d1A,d1B
//		makeProtocolTimingSpec< 12, 20,  320,    1, 36,    1,  2,    2,  1>::RX, // (SM5212)
//		makeProtocolTimingSpec< 11, 20,  270,    1, 36,    1,  2,    2,  1>::RX, // (HT12E)
//		makeProtocolTimingSpec< 10, 20,  365,    1, 18,    3,  1,    1,  3>::RX, // (1ByOne Doorbell)
//		makeProtocolTimingSpec<  6, 20,  450,    1, 23,    1,  2,    2,  1>::RX, // (HT6P20B)
//};
static const RxTimingSpecTable <
//     		                 #,  %,  clk,  syA,syB,  d0A,d0B,  d1A,d1B
	makeProtocolTimingSpec< 12, 20,  320,    1, 36,    1,  2,    2,  1>, // (SM5212)
	makeProtocolTimingSpec< 11, 20,  270,    1, 36,    1,  2,    2,  1>, // (HT12E)
	makeProtocolTimingSpec< 10, 20,  365,    1, 18,    3,  1,    1,  3>, // (1ByOne Doorbell)
	makeProtocolTimingSpec<  6, 20,  450,    1, 23,    1,  2,    2,  1>  // (HT6P20B)
> inverseLevelProtocolsTable;


/* The number of rows of the 2 above tables. */

constexpr size_t  normalLevelProtocolsTableRowCount = normalLevelProtocolsTable.ELEMENT_COUNT;
bool RxTimingSpec::isNormalLevelProtocol() const {
	const bool result = this < (&normalLevelProtocolsTable.toArray()[normalLevelProtocolsTableRowCount]) &&
			(this >= normalLevelProtocolsTable.toArray());
	return result;
}

constexpr size_t inverseLevelProtocolsTableRowCount = inverseLevelProtocolsTable.ELEMENT_COUNT;
bool RxTimingSpec::isInverseLevelProtocol() const {
	const bool result = this < (&inverseLevelProtocolsTable.toArray()[inverseLevelProtocolsTableRowCount]) &&
			(this >= inverseLevelProtocolsTable.toArray());
	return result;
}

/** Returns the array of protocols for a protocol group. */
std::pair<const RxTimingSpec*, size_t> getRxTimingTable(const size_t protocolGroupId) {
	static const std::pair<const RxTimingSpec*, size_t> protocolGroups[] = {
			{ normalLevelProtocolsTable.toArray(),  normalLevelProtocolsTableRowCount},
			{inverseLevelProtocolsTable.toArray(), inverseLevelProtocolsTableRowCount},
	};

	return protocolGroups[protocolGroupId];
}

#if DEBUG_RCSWITCH_PROTOCOL_SPEC

void printRxTimingTable(UARTClass& serial, const size_t protocolGroup) {
	std::pair<const RxTimingSpec*, size_t> pt = RcSwitch::getRxTimingTable(protocolGroup);
	for (size_t i = 0; i < pt.second; i++) {
		const RxTimingSpec &p = (pt.first)[i];
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
#endif

} /* namespace RcSwitch */
