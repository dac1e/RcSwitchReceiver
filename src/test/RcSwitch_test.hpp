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

#ifndef UNITTEST_RCSWITCHRECEIVERTEST_HPP_
#define UNITTEST_RCSWITCHRECEIVERTEST_HPP_

#define ENABLE_RCSWITCH_TEST true

#if ENABLE_RCSWITCH_TEST

#include "../internal/RcSwitch.hpp"

namespace RcSwitch {

struct TxDataBit {
	DATA_BIT mDataBit;
	// Force transmission error by pulse duration manipulation
	double mFirstPulseDurationFactor;
	double mSecondPulseDurationFactor;

	TxDataBit(const DATA_BIT dataBit)
		: mDataBit(dataBit)
		, mFirstPulseDurationFactor(1.0)
		, mSecondPulseDurationFactor(1.0) {
	}

	TxDataBit(const DATA_BIT dataBit, double firstPulseDurationFactor
			, double secondPulseDurationFactor)
		: mDataBit(dataBit)
		, mFirstPulseDurationFactor(firstPulseDurationFactor)
		, mSecondPulseDurationFactor(secondPulseDurationFactor) {
	}
};

typedef RxProtocolTable <
//                           #,  %,  clk,syA,  syB,  d0A,d0B,  d1A,d1B , inverseLevel
	makeTimingSpec<  1, 20,  350,  1,   31,    1,  3,    3,  1>, // ()
	makeTimingSpec<  2, 20,  650,  1,   10,    1,  3,    3,  1>, // ()
	makeTimingSpec<  3, 20,  100, 30,   71,    4, 11,    9,  6>, // ()
	makeTimingSpec<  4, 20,  380,  1,    6,    1,  3,    3,  1>, // ()
	makeTimingSpec<  5, 20,  500,  6,   14,    1,  2,    2,  1>, // ()
	makeTimingSpec<  6, 20,  450,  1,   23,    1,  2,    2,  1, true>, 	// (HT6P20B)
	makeTimingSpec<  7, 20,  150,  2,   62,    1,  6,    6,  1>, 		// (HS2303-PT)
	makeTimingSpec<  8, 20,  200,  3,  130,    7, 16,    3, 16>, 		// (Conrad RS-200 RX)

	makeTimingSpec< 10, 20,  365,    1, 18,    3,  1,    1,  3, true>, 	// (1ByOne Doorbell)
	makeTimingSpec< 11, 20,  270,    1, 36,    1,  2,    2,  1, true>, 	// (HT12E)
	makeTimingSpec< 12, 20,  320,    1, 36,    1,  2,    2,  1, true>  	// (SM5212)
> rxTestProtocolTable;


class RcSwitch_test {
public:
	static void handleInterrupt(Receiver& receiver, const int pinLevel
		, const uint32_t microSecInterruptTime)
	{
		return receiver.handleInterrupt(pinLevel, microSecInterruptTime);
	}

	static void sendDataPulse(uint32_t &usec
		, Receiver &receiver
		, const uint32_t firstPulse
		, const uint32_t secondPulse
		, const uint32_t firstPulseEndLevel);

private:
	/* Send a message package multiple times */
	void sendMessagePacket(uint32_t &usec, Receiver &receiver
		, const TxDataBit* const dataBits
		, const size_t count) const;

	void faultyMessagePacketTest(uint32_t& usec, Receiver &receiver
		, const TxDataBit* const faultyMessagePacket) const;

	void tooShortMessagePacketTest(uint32_t& usec, Receiver &receiver) const;

	void testStackBuffer() const;
	void testRingBuffer() const;
	void testProtocolCandidates() const;
	void testSynchRx() const;
	void testDataRx() const;
	void testFaultyDataRx() const;

public:
	void run() const{
		testStackBuffer();
		testRingBuffer();
		testProtocolCandidates();
		testSynchRx();
		testDataRx();
		testFaultyDataRx();
	}

	static RcSwitch_test theTest;
};

} // namespace RcSwitch

#endif // ENABLE_RCSWITCH_TEST

#endif /* UNITTEST_RCSWITCHRECEIVERTEST_HPP_ */
