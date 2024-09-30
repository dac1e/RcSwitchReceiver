/*
 * RcSwitchTest.hpp
 *
 *  Created on: 23.09.2024
 *      Author: Wolfgang
 */
#pragma once

#ifndef UNITTEST_RCSWITCHRECEIVERTEST_HPP_
#define UNITTEST_RCSWITCHRECEIVERTEST_HPP_

#define ENABLE_RCSWITCH_TEST false

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

	void testBlockingStack() const;
	void testOverwritingStack() const;
	void testProtocolCandidates() const;
	void testSynchRx() const;
	void testDataRx() const;
	void testFaultyDataRx() const;

public:
	void run() const{
		testBlockingStack();
		testOverwritingStack();
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
