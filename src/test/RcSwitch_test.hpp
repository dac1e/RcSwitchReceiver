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

#ifndef RCSWITCH_RECEIVER_TEST_RCSWITCH_TEST_HPP_
#define RCSWITCH_RECEIVER_TEST_RCSWITCH_TEST_HPP_

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

class RcSwitch_test {
public:
	static void handleInterrupt(Receiver& receiver, const int pinLevel
		, const uint32_t usecInterruptEntry)
	{
		return receiver.handleInterrupt(pinLevel, usecInterruptEntry);
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

#endif /* RCSWITCH_RECEIVER_TEST_RCSWITCH_TEST_HPP_ */
