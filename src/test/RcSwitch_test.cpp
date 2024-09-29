/*
 * RcSwitchTest.cpp
 *
 *  Created on: 23.09.2024
 *      Author: Wolfgang
 */

#define ENABLE_RCSWITCH_TEST false
#if ENABLE_RCSWITCH_TEST

#include <assert.h>
#include "RcSwitch_test.hpp"

/** Call rcSwitchTest.run() to execute tests. */
RcSwitch::RcSwitch_test rcSwitchTest;

namespace RcSwitch {

static const TxDataBit validMessagePacket_A[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static_assert( sizeof(validMessagePacket_A) / sizeof(validMessagePacket_A[0]) > MIN_MSG_PACKET_BITS,
		"Insufficient data bits for a valid message packet.");

static const TxDataBit validMessagePacket_B[] = {
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static_assert( sizeof(validMessagePacket_B) / sizeof(validMessagePacket_B[0]) > MIN_MSG_PACKET_BITS,
		"Insufficient data bits for a valid message packet.");

static const TxDataBit invalidMessagePacket_firstPulseTooShort[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0, 0.3, 1.0 /* Send a too short first pulse */},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static const TxDataBit invalidMessagePacket_firstPulseTooLong[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0, 1.3, 1.0 /* Send a too long first pulse */},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static const TxDataBit invalidMessagePacket_secondPulseTooShort[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0, 1.0, 0.3 /* Send a too short second pulse */},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static const TxDataBit invalidMessagePacket_secondPulseTooLong[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0, 1.0, 1.3 /* Send a too long second pulse */},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

#if DEBUG_RCSWITCH
template<> inline const int& INITIAL_VALUE<int>() {
	static const int value = std::numeric_limits<int>::max();
	return value;
}
#endif

template<size_t protocolNumber> class PulseLength {};

template<> struct PulseLength<1> {
	static constexpr uint32_t synchShortPulseLength = 300;
	static constexpr uint32_t synchLongPulseLength = 9300;
	static constexpr uint32_t shortPulseLength =      150;
	static constexpr uint32_t longPulseLength  =      900;
	/**
	 * The target level of the edge of the pulse.
	 */
	static constexpr uint32_t firstPulseEndLevel  =     0;
};

void RcSwitch_test::sendDataPulse(uint32_t &usec, Receiver &receiver, const uint32_t firstPulse
		, const uint32_t secondPulse, const uint32_t firstPulseEndLevel) {

	const uint32_t secondPulseEndLevel = not firstPulseEndLevel;

	usec += firstPulse;
	receiver.handleInterrupt(firstPulseEndLevel, usec);

	usec += secondPulse;
	receiver.handleInterrupt(secondPulseEndLevel, usec);
}

template<size_t protocolNumber> struct Protocol {
	static void sendLogical0(uint32_t &usec, Receiver &receiver,
			const double& firstPulseDurationFactor, const double& secondPulseDurationFactor) {

		const uint32_t firstPulseEndLevel  = PulseLength<protocolNumber>::firstPulseEndLevel;

		const uint32_t shortPulseLength = PulseLength<protocolNumber>::shortPulseLength;
		const uint32_t longPulseLength  = PulseLength<protocolNumber>::longPulseLength;

		RcSwitch_test::sendDataPulse(usec, receiver,
				shortPulseLength * firstPulseDurationFactor,
				longPulseLength  * secondPulseDurationFactor,
				firstPulseEndLevel);
	}

	static void sendLogical1(uint32_t &usec, Receiver &receiver,
			const double& firstPulseDurationFactor, const double& secondPulseDurationFactor) {

		const uint32_t firstPulseEndLevel  = PulseLength<protocolNumber>::firstPulseEndLevel;

		const uint32_t shortPulseLength = PulseLength<protocolNumber>::shortPulseLength;
		const uint32_t longPulseLength  = PulseLength<protocolNumber>::longPulseLength;

		RcSwitch_test::sendDataPulse(usec, receiver,
				longPulseLength  * firstPulseDurationFactor,
				shortPulseLength * secondPulseDurationFactor,
				firstPulseEndLevel);
	}

	static void sendSynchPulses(uint32_t& usec, Receiver& receiver) {
		const uint32_t firstPulseEndLevel  = PulseLength<protocolNumber>::firstPulseEndLevel;
		const uint32_t secondPulseEndLevel = not firstPulseEndLevel;

		usec += PulseLength<protocolNumber>::synchShortPulseLength;
		RcSwitch_test::handleInterrupt(receiver, firstPulseEndLevel, usec);

		usec += PulseLength<protocolNumber>::synchLongPulseLength;
		RcSwitch_test::handleInterrupt(receiver, secondPulseEndLevel, usec);
	}

	static void sendDataBit(uint32_t &usec, Receiver &receiver, const TxDataBit* const dataBit) {
		switch(dataBit->mDataBit) {
			case DATA_BIT::LOGICAL_0:
				sendLogical0(usec, receiver,
						dataBit->mFirstPulseDurationFactor, dataBit->mSecondPulseDurationFactor);
				break;
			case DATA_BIT::LOGICAL_1:
				sendLogical1(usec, receiver,
						dataBit->mFirstPulseDurationFactor, dataBit->mSecondPulseDurationFactor);
				break;
			case DATA_BIT::UNKNOWN:
				break;
		}
	}
};

void RcSwitch_test::sendMessagePacket(uint32_t &usec, Receiver &receiver
		, const TxDataBit* const dataBits, const size_t count) {

	for(size_t i = 0; i < count; i++) {
		Protocol<1>::sendSynchPulses(usec, receiver);

		if(!receiver.available()) {
			if(i < MIN_MSG_PACKET_REPEATS) {
				assert(receiver.state() == Receiver::DATA_STATE);
			}

			for(size_t j = 0; dataBits[j].mDataBit != DATA_BIT::UNKNOWN; j++) {
				Protocol<1>::sendDataBit(usec, receiver, &dataBits[j]);
			}
		}
	}
}

void RcSwitch_test::faultyMessagePacketTest(uint32_t& usec, Receiver &receiver,
		const TxDataBit* const faultyMessagePacket) {
	{
		// Send first pulse too short faulty message
		sendMessagePacket(usec, receiver, faultyMessagePacket, 1);
		assert(receiver.state() == Receiver::SYNC_STATE);
		assert(receiver.mProtocolCandidates.size() == 0);
	}
	{
		// Send valid message
		sendMessagePacket(usec, receiver, validMessagePacket_A, MIN_MSG_PACKET_REPEATS + 1);
		assert(receiver.available());
		const uint32_t receivedValue = receiver.receivedValue();
		/* binary: 010011 */
		assert(receivedValue == 0x13 /* binary: 010011 */);
	}
}

void RcSwitch_test::testFaultyDataRx() {
	Receiver receiver;
	uint32_t usec = 0;

	usec += 100; // start hi pulse 100 usec duration.
	receiver.handleInterrupt(not PulseLength<1>::firstPulseEndLevel, usec);

	faultyMessagePacketTest(usec, receiver, invalidMessagePacket_firstPulseTooShort);
	receiver.reset();
	faultyMessagePacketTest(usec, receiver, invalidMessagePacket_firstPulseTooLong);
	receiver.reset();
	faultyMessagePacketTest(usec, receiver, invalidMessagePacket_secondPulseTooShort);
	receiver.reset();
	faultyMessagePacketTest(usec, receiver, invalidMessagePacket_secondPulseTooLong);
	receiver.reset();
}

void RcSwitch_test::testDataRx() {
	Receiver receiver;
	uint32_t usec = 0;

	usec += 100; // start hi pulse 100 usec duration.
	receiver.handleInterrupt(not PulseLength<1>::firstPulseEndLevel, usec);

	{ // Send valid message
		sendMessagePacket(usec, receiver, validMessagePacket_A, MIN_MSG_PACKET_REPEATS + 1);
		assert(receiver.available());

		const uint32_t receivedValue = receiver.receivedValue();
		assert(receivedValue == 0x13 /* binary: 010011 */); // Confirm received value.
	}

	{ // Send a different valid message, without resetting old received value.
		sendMessagePacket(usec, receiver, validMessagePacket_B, MIN_MSG_PACKET_REPEATS + 1);
		assert(receiver.available());

		const uint32_t receivedValue = receiver.receivedValue();
		assert(receivedValue == 0x13 /* binary: 010011 */); // Confirm old received value.
	}

	{ // Send a different valid message, with resetting the old received before.
		receiver.reset();
		sendMessagePacket(usec, receiver, validMessagePacket_B, MIN_MSG_PACKET_REPEATS + 1);
		assert(receiver.available());

		const uint32_t receivedValue = receiver.receivedValue();
		assert(receivedValue == 0x2C /* binary: 101100 */); // Confirm new received value.
	}
}

void RcSwitch_test::testSynchRx() {
	Receiver receiver;
	uint32_t usec = 0;

	usec += 100;  // start hi pulse  100 usec duration.
	receiver.handleInterrupt(1, usec);

	usec += 300;  // start lo pulse 300 usec duration.
	receiver.handleInterrupt(0, usec);

	usec += 2736; // start lo pulse 2736 usec duration, too short for any protocol.
	receiver.handleInterrupt(1, usec);
	assert(receiver.state() == Receiver::SYNC_STATE);

	usec += 100;  // start lo pulse 100 usec duration.
	receiver.handleInterrupt(0, usec);

	usec += 300;  // start hi pulse 300 usec duration.
	receiver.handleInterrupt(1, usec);

	usec += 9300; // start lo pulse 9300 usec duration, match protocol #1 and #7.
	receiver.handleInterrupt(0, usec);
	assert(receiver.state() == Receiver::DATA_STATE);
}

void RcSwitch_test::testProtocolCandidates() {
	Receiver receiver;

	Pulse pulse_0 = {																				// Hi pulse too short
			239, PULSE_LEVEL::HI
	};

	Pulse pulse_1 = {																				// Lo pulse within protocol #1 and #7
			10850, PULSE_LEVEL::LO
	};

	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 0); 			// No matching protocol

	pulse_0.mMicroSecDuration = 280;
	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 2); 							// Match protocol #1 and #7
	for(size_t i = 0; i < receiver.mProtocolCandidates.size(); i++) {  // Check protocol candidates
		static const size_t expectedProtocols[] = {7,1};
		assert(receiver.mProtocolCandidates.getProtcolNumber(i) == expectedProtocols[i]);
	}

	receiver.mProtocolCandidates.reset();										// Remove protocol candidates
	pulse_1.mMicroSecDuration = 7439;												// Lo pulse too short
	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 0); 			// No matching protocol

	pulse_0.mMicroSecDuration = 360;												// Hi pulse matches protocols #1 and #4, Lo pulse still too short.
	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 0); 			// No matching protocol

	pulse_1.mMicroSecDuration = 2735;												// Lo pulse matches protocol #4
	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 1); 			// No matching protocol
	for(size_t i = 0; i < receiver.mProtocolCandidates.size(); i++) {  // Check protocol candidates
		static const size_t expectedProtocols[] = {4};
		assert(receiver.mProtocolCandidates.getProtcolNumber(i) == expectedProtocols[i]);
	}
}

void RcSwitch_test::testBlockingStack() {
	constexpr int start = -2;
	constexpr int end = 3;

	BlockingStack<int, end - start, volatile size_t> blockingStack;

	int e = start;
	for(; e < end; e++) { 																	// fill stack elements with -2 .. 3.
		assert(blockingStack.size() == static_cast<size_t>(e-start));
		blockingStack.push(e);
		assert(blockingStack[e-start] == e);
		assert(blockingStack.overflowCount() == 0);
	}

	assert(blockingStack.size() == end-start); 							// stack should be full.
	assert(blockingStack.push(e) == false); 								// element should be dropped.
	assert(blockingStack.size() == blockingStack.capacity); // stack should still be full.
	assert(blockingStack.overflowCount() == 1); 						// overflow should be raised.

	blockingStack.remove(2); 																	// remove the middle element.
	assert(blockingStack.size() == blockingStack.capacity-1); // expect one element less.

	for(size_t i = 0; i < blockingStack.size(); i++) {      // check the remaining elements.
		static const int expected[] = {-2,-1,1,2};
		assert(blockingStack[i] == expected[i]);
	}

	assert(blockingStack.overflowCount() == 1); 						// overflow should still be raised.
	assert(blockingStack.size() == blockingStack.capacity - 1);
	blockingStack.push(e);   																// push should be successful.
	assert(blockingStack.size() == blockingStack.capacity);
	blockingStack.reset(); 																	// remove all elements.
	assert(blockingStack.size() == 0); 											// size should be zero.
	assert(blockingStack.overflowCount() == 0); 					// overflow should be reset.
}

void RcSwitch_test::testOverwritingStack() {
	constexpr int start = -2;
	constexpr int end = 3;

	OverwritingStack<int, end - start, volatile size_t> overwritingStack;

	int e = start;
	for(; e < end; e++) { 																	// fill stack elements with -2 .. 3.
		assert(overwritingStack.size() == static_cast<size_t>(e-start));
		overwritingStack.push(e);
		assert(overwritingStack[e-start] == e);
	}

	assert(overwritingStack.size() == end-start); 							// stack should be full.
	overwritingStack.push(e++); 															  // element should overwrite the oldest one.
	assert(overwritingStack.size() == overwritingStack.capacity); // stack should still be full.

	for(size_t i = 0; i < overwritingStack.size(); i++) {      // check the elements.
		static const int expected[] = {-1,0,1,2,3};
		assert(overwritingStack[i] == expected[i]);
	}

	overwritingStack.push(e++); 															  // element should overwrite the oldest one.
	assert(overwritingStack.size() == overwritingStack.capacity); // stack should still be full.

	for(size_t i = 0; i < overwritingStack.size(); i++) {      // check the elements.
		static const int expected[] = {0,1,2,3,4};
		assert(overwritingStack[i] == expected[i]);
	}
}

} /* namespace RcSwitch */

#endif // ENABLE_RCSWITCH_TEST
