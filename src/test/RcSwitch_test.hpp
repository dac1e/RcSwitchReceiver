/*
 * RcSwitchTest.hpp
 *
 *  Created on: 23.09.2024
 *      Author: Wolfgang
 */
#pragma once

#ifndef UNITTEST_RCSWITCHRECEIVERTEST_HPP_
#define UNITTEST_RCSWITCHRECEIVERTEST_HPP_

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
		, const size_t count);

	void faultyMessagePacketTest(uint32_t& usec, Receiver &receiver
		, const TxDataBit* const faultyMessagePacket);

	void testBlockingStack();
	void testOverwritingStack();
	void testProtocolCandidates();
	void testSynchRx();
	void testDataRx();
	void testFaultyDataRx();

public:
	void run() {
		testBlockingStack();
		testOverwritingStack();
		testProtocolCandidates();
		testSynchRx();
		testDataRx();
		testFaultyDataRx();
	}
};

} // namespace RcSwitch

#endif /* UNITTEST_RCSWITCHRECEIVERTEST_HPP_ */
