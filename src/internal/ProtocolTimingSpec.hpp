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

#ifndef RCSWITCH_RECEIVER_INTERNAL_PROTOCOL_TIMING_SPEC_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_PROTOCOL_TIMING_SPEC_HPP_

#include <stddef.h>
#include <stdint.h>

#include "ISR_ATTR.hpp"
#include "RxTimingSpecTable.hpp"
#include "Typeselect.hpp"
#include <Arduino.h>


namespace RcSwitch {

struct TimeRange {
	unsigned int lowerBound;
	unsigned int upperBound;

	enum COMPARE_RESULT {
		IS_WITHIN =  0,
		TOO_SHORT = -1,
		TOO_LONG  = -1,
	};

	COMPARE_RESULT compare(uint32_t value) const;
};

inline TimeRange::COMPARE_RESULT TimeRange::compare(uint32_t value) const {
	if(value <  lowerBound) {return TOO_SHORT;}
	if(value >= upperBound) {return TOO_LONG;}
	return IS_WITHIN;
}

struct RxPulsePairTimeRanges {
	TimeRange durationA;
	TimeRange durationB;
};

struct RxTimingSpec {
	unsigned int   protocolNumber;
	bool bInverseLevel;
	RxPulsePairTimeRanges  synchronizationPulsePair;
	RxPulsePairTimeRanges  data0pulsePair;
	RxPulsePairTimeRanges  data1pulsePair;
};

struct TxPulsePairTiming {
	unsigned int durationA;
	unsigned int durationB;
};

template<typename L, typename R> struct isRxTimingSpecLower {
	static constexpr bool value = L::INVERSE_LEVEL == R::INVERSE_LEVEL ?
			(L::usecSynchA_lowerBound < R::usecSynchA_lowerBound) : L::INVERSE_LEVEL < R::INVERSE_LEVEL;
};

namespace Debug {
	typedef typeof(Serial) serial_t;
	void dumpRxTimingSpecTable(serial_t &serial, const RxTimingSpecTable &rxtimingSpecTable);
}

} // namespace RcSwitch

/**
 * makeTimingSpec
 */
template<
	unsigned int protocolNumber,
	unsigned int usecClock,
	unsigned percentTolerance,
	unsigned int synchA,  unsigned int synchB,
	unsigned int data0_A, unsigned int data0_B,
	unsigned int data1_A, unsigned int data1_B,
	bool inverseLevel>

struct makeTimingSpec { // Calculate the timing specification from the protocol definition.
	static constexpr unsigned int PROTOCOL_NUMBER = protocolNumber;
	static constexpr bool INVERSE_LEVEL = inverseLevel;

	static constexpr unsigned int uSecSynchA = usecClock * synchA;
	static constexpr unsigned int uSecSynchB = usecClock * synchB;
	static constexpr unsigned int usecSynchA_lowerBound = static_cast<uint32_t>(uSecSynchA) * (100-percentTolerance) / 100;
	static constexpr unsigned int usecSynchA_upperBound = static_cast<uint32_t>(uSecSynchA) * (100+percentTolerance) / 100;
	static constexpr unsigned int usecSynchB_lowerBound = static_cast<uint32_t>(uSecSynchB) * (100-percentTolerance) / 100;
	static constexpr unsigned int usecSynchB_upperBound = static_cast<uint32_t>(uSecSynchB) * (100+percentTolerance) / 100;

	static constexpr unsigned int uSecData0_A = usecClock * data0_A;
	static constexpr unsigned int uSecData0_B = usecClock * data0_B;
	static constexpr unsigned int uSecData0_A_lowerBound = static_cast<uint32_t>(uSecData0_A) * (100-percentTolerance) / 100;
	static constexpr unsigned int uSecData0_A_upperBound = static_cast<uint32_t>(uSecData0_A) * (100+percentTolerance) / 100;
	static constexpr unsigned int uSecData0_B_lowerBound = static_cast<uint32_t>(uSecData0_B) * (100-percentTolerance) / 100;
	static constexpr unsigned int uSecData0_B_upperBound = static_cast<uint32_t>(uSecData0_B) * (100+percentTolerance) / 100;

	static constexpr unsigned int uSecData1_A = usecClock * data1_A;
	static constexpr unsigned int uSecData1_B = usecClock * data1_B;
	static constexpr unsigned int uSecData1_A_lowerBound = static_cast<uint32_t>(uSecData1_A) * (100-percentTolerance) / 100;
	static constexpr unsigned int uSecData1_A_upperBound = static_cast<uint32_t>(uSecData1_A) * (100+percentTolerance) / 100;
	static constexpr unsigned int uSecData1_B_lowerBound = static_cast<uint32_t>(uSecData1_B) * (100-percentTolerance) / 100;
	static constexpr unsigned int uSecData1_B_upperBound = static_cast<uint32_t>(uSecData1_B) * (100+percentTolerance) / 100;

	typedef RcSwitch::RxTimingSpec rx_spec_t;
	static constexpr rx_spec_t RX = {PROTOCOL_NUMBER, INVERSE_LEVEL,
		{	/* synch pulses */
			{usecSynchA_lowerBound, usecSynchA_upperBound},     {usecSynchB_lowerBound, usecSynchB_upperBound}
		},
		{   /* LOGICAL_0 data bit pulses */
			{uSecData0_A_lowerBound, uSecData0_A_upperBound}, {uSecData0_B_lowerBound, uSecData0_B_upperBound}
		},
		{
			/* LOGICAL_1 data bit pulses */
			{uSecData1_A_lowerBound, uSecData1_A_upperBound}, {uSecData1_B_lowerBound, uSecData1_B_upperBound}
		},
	};

	template<typename T> struct IS_RX_LOWER {
		static constexpr bool value = inverseLevel == T::inverseLevel ?
				(usecSynchA_lowerBound < T::usecSynchA_lowerBound) : inverseLevel < T::inverseLevel;
	};
};


/**
 * RxProtocolTable
 */
template<typename ...TimingSpecs> struct RxProtocolTable;

template<typename ...Ts> struct
RxProtocolTable {
private:
	using T = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::selected;
	using R = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::rest;
	const RcSwitch::RxTimingSpec* toArray() const {return &m;}
public:
	static constexpr size_t ROW_COUNT =	sizeof(RxProtocolTable) / sizeof(RcSwitch::RxTimingSpec);
	RcSwitch::RxTimingSpec m = T::RX;
	RxProtocolTable<R> r;

	/* Convert to rxTimingSpecTable */
	inline RcSwitch::RxTimingSpecTable toTimingSpecTable() const {
		constexpr size_t rowCount = ROW_COUNT;
		return RcSwitch::RxTimingSpecTable{toArray(), rowCount};
	}
	inline void dumpTimingSpec(RcSwitch::Debug::serial_t &serial) const {
		RcSwitch::Debug::dumpRxTimingSpecTable(serial, toTimingSpecTable());
	}
};

/**
 * RxProtocolTable specialization for a table with just 1 row.
 */
template<typename T> struct
RxProtocolTable<T> {
private:
	const RcSwitch::RxTimingSpec* toArray() const {return &m;}
public:
	static constexpr size_t ROW_COUNT =	sizeof(RxProtocolTable) / sizeof(RcSwitch::RxTimingSpec);
	RcSwitch::RxTimingSpec m = T::RX;

	/* Convert to rxTimingSpecTable */
	inline RcSwitch::RxTimingSpecTable toTimingSpecTable() const {
		constexpr size_t rowCount = ROW_COUNT;
		return RcSwitch::RxTimingSpecTable{toArray(), rowCount};
	}
	inline void dumpTimingSpec(RcSwitch::Debug::serial_t &serial) const {
		RcSwitch::Debug::dumpRxTimingSpecTable(serial, toTimingSpecTable());
	}
};

/**
 * For internal use only.
 * RxProtocolTable specialization for timing specs being passed as a tuple.
 */
template<typename ...Ts> struct
RxProtocolTable<typeselect::tuple<Ts...>> {
private:
	using T = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::selected;
	using R = typename typeselect::select<RcSwitch::isRxTimingSpecLower, Ts...>::rest;
public:
	RcSwitch::RxTimingSpec m = T::RX;
	RxProtocolTable<R> r;
};

/**
 * For internal use only.
 * RxProtocolTable specialization for a single timing spec being passed as a tuple.
 */
template<typename T> struct
RxProtocolTable<typeselect::tuple<T>> {
	RcSwitch::RxTimingSpec m = T::RX;
};

#endif // RCSWITCH_RECEIVER_INTERNAL_PROTOCOL_TIMING_SPEC_HPP_
